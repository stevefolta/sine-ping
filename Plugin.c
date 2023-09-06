#include "Plugin.h"
#include "Parameters.h"
#include "Stream.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static bool Plugin_init(const clap_plugin_t* clap_plugin)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	for (int i = 0; i < NUM_VOICES; ++i)
		Voice_init(&self->voices[i], self);
	for (int param_id = 0; param_id < NUM_PARAMS; ++param_id)
		self->params[param_id] = param_info[param_indices[param_id]].default_value;
	return true;
}

static void Plugin_destroy(const clap_plugin_t* clap_plugin)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	free(self);
}

static bool Plugin_activate(
	const clap_plugin_t* clap_plugin,
	double sample_rate,
	uint32_t minimum_frames_count, uint32_t maximum_frames_count)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	self->sample_rate = sample_rate;
	return true;
}

static void Plugin_deactivate(const clap_plugin_t* clap_plugin)
{
}

static bool Plugin_start_processing(const clap_plugin_t* clap_plugin)
{
	return true;
}

static void Plugin_stop_processing(const clap_plugin_t* clap_plugin)
{
}

static void Plugin_reset(const clap_plugin_t* clap_plugin)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	for (int i = 0; i < NUM_VOICES; ++i)
		Voice_reset(&self->voices[i]);
}

static void Plugin_render(Plugin* self, uint32_t num_frames, float* l_out, float* r_out)
{
	memset(l_out, 0, num_frames * sizeof(float));
	memset(r_out, 0, num_frames * sizeof(float));
	for (int i = 0; i < NUM_VOICES; ++i)
		Voice_render(&self->voices[i], num_frames, l_out, r_out);
}

static void Plugin_process_event(Plugin* self, const clap_event_header_t* event)
{
	if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
		const clap_event_note_t* note_event;
		switch (event->type) {
			case CLAP_EVENT_NOTE_ON:
				note_event = (clap_event_note_t*) event;
				for (int i = 0; i < NUM_VOICES; ++i) {
					if (Voice_is_free(&self->voices[i])) {
						Voice_start_note(&self->voices[i], note_event->note_id, note_event->channel, note_event->key);
						break;
						}
					}
				break;

			case CLAP_EVENT_NOTE_OFF:
			case CLAP_EVENT_NOTE_CHOKE:
				note_event = (clap_event_note_t*) event;
				for (int i = 0; i < NUM_VOICES; ++i) {
					if (Voice_is_for_note(&self->voices[i], note_event->note_id, note_event->channel, note_event->key)) {
						if (event->type == CLAP_EVENT_NOTE_OFF)
							Voice_end_note(&self->voices[i]);
						else
							Voice_choke_note(&self->voices[i]);
						}
					}
				break;

			case CLAP_EVENT_PARAM_VALUE:
				{
				const clap_event_param_value_t* value_event = (const clap_event_param_value_t*) event;
				if (value_event->param_id >= NUM_PARAMS)
					break;
				self->params[value_event->param_id] = value_event->value;
				}
				break;

			case CLAP_EVENT_PARAM_MOD:
				{
				const clap_event_param_mod_t* mod_event = (const clap_event_param_mod_t*) event;
				if (mod_event->param_id >= NUM_PARAMS)
					break;
				for (int i = 0; i < NUM_VOICES; ++i) {
					Voice* voice = &self->voices[i];
					if (Voice_is_for_note(voice, mod_event->note_id, mod_event->channel, mod_event->key))
						voice->param_offsets[mod_event->param_id] = mod_event->amount;
					}
				}
				break;
			}
		}
}

static void Plugin_send_all_parameters_to_host(const clap_plugin_t* clap_plugin, const clap_output_events_t* out);

static clap_process_status Plugin_process(
	const clap_plugin_t* clap_plugin, 
	const clap_process_t* process)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	const uint32_t num_frames = process->frames_count;
	const uint32_t num_events = process->in_events->size(process->in_events);
	uint32_t cur_event = 0;
	uint32_t next_event_frame = num_events > 0 ? 0 : num_frames;

	// Since we never change parameters on our own, the only time we need to send
	// them to the host is after loading them all from the state.
	// ...Actually, not even then.
	if (self->need_to_send_params_to_host) {
		Plugin_send_all_parameters_to_host(clap_plugin, process->out_events);
		self->need_to_send_params_to_host = false;
		}

	// Render and handle events.
	for (uint32_t cur_frame = 0; cur_frame < num_frames; ) {
		// Handle events at this frame (and/or update next_event_frame).
		while (cur_event < num_events && next_event_frame == cur_frame) {
			const clap_event_header_t* event = process->in_events->get(process->in_events, cur_event);
			if (event->time != cur_frame) {
				next_event_frame = event->time;
				break;
				}

			Plugin_process_event(self, event);

			cur_event += 1;
			if (cur_event == num_events) {
				next_event_frame = num_frames;
				break;
				}
			}

		// Render.
		Plugin_render(
			self,
			next_event_frame - cur_frame,
			process->audio_outputs[0].data32[0] + cur_frame, 
			process->audio_outputs[0].data32[1] + cur_frame);
		cur_frame = next_event_frame;
		}

	// Send note-end events back to the host.
	for (int i = 0; i < NUM_VOICES; ++i) {
		Voice* voice = &self->voices[i];
		if (Voice_just_ended(voice)) {
			clap_event_note_t event = {};
			event.header.size = sizeof(event);
			event.header.time = 0;
			event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			event.header.type = CLAP_EVENT_NOTE_END;
			event.header.flags = 0;
			event.key = voice->key;
			event.note_id = voice->note_id;
			event.channel = voice->channel;
			event.port_index = 0;
			process->out_events->try_push(process->out_events, &event.header);
			}
		}

	return CLAP_PROCESS_CONTINUE;
}

static uint32_t Plugin_note_ports_count(const clap_plugin_t* clap_plugin, bool is_input);
static uint32_t Plugin_audio_ports_count(const clap_plugin_t* clap_plugin, bool is_input);
static bool Plugin_get_note_port(const clap_plugin_t* clap_plugin, uint32_t index, bool is_input, clap_note_port_info_t* info);
static bool Plugin_get_audio_port(const clap_plugin_t* clap_plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info);
static uint32_t Plugin_params_count(const clap_plugin_t* clap_plugin);
static bool Plugin_get_param_info(const clap_plugin_t* clap_plugin, uint32_t index, clap_param_info_t* info_out);
static bool Plugin_get_param_value(const clap_plugin_t* clap_plugin, clap_id id, double* value_out);
static bool Plugin_param_to_text(const clap_plugin_t* clap_plugin, clap_id id, double value, char* str_out, uint32_t size);
static bool Plugin_param_text_to_value(const clap_plugin_t* clap_plugin, clap_id id, const char* str, double* value_out);
static void Plugin_flush_params(const clap_plugin_t* clap_plugin, const clap_input_events_t* events_in, const clap_output_events_t* events_out);
static bool Plugin_save_state(const clap_plugin_t* clap_plugin, const clap_ostream_t* stream);
static bool Plugin_load_state(const clap_plugin_t* clap_plugin, const clap_istream_t* stream);

static const void* Plugin_get_extension(const clap_plugin_t* clap_plugin, const char* id)
{
	static const clap_plugin_note_ports_t note_ports_extension = {
		.count = Plugin_note_ports_count,
		.get = Plugin_get_note_port,
		};
	static const clap_plugin_audio_ports_t audio_ports_extension = {
		.count = Plugin_audio_ports_count,
		.get = Plugin_get_audio_port,
		};
	static const clap_plugin_params_t params_extension = {
		.count = Plugin_params_count,
		.get_info = Plugin_get_param_info,
		.get_value = Plugin_get_param_value,
		.value_to_text = Plugin_param_to_text,
		.text_to_value = Plugin_param_text_to_value,
		.flush = Plugin_flush_params,
		};
	static const clap_plugin_state_t state_extension = {
		.save = Plugin_save_state,
		.load = Plugin_load_state,
		};

	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &note_ports_extension;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &audio_ports_extension;
	if (strcmp(id, CLAP_EXT_PARAMS) == 0)
		return &params_extension;
	if (strcmp(id, CLAP_EXT_STATE) == 0)
		return &state_extension;
	return NULL;
}

static void Plugin_main_thread_tick(const clap_plugin_t* clap_plugin)
{
	// Plugin* self = (Plugin*) clap_plugin->plugin_data;
	//***
}

extern const clap_plugin_descriptor_t descriptor;
static const clap_plugin_t clap_plugin_template = {
	.desc = &descriptor,
	.init = Plugin_init,
	.destroy = Plugin_destroy,
	.activate = Plugin_activate,
	.deactivate = Plugin_deactivate,
	.start_processing = Plugin_start_processing,
	.stop_processing = Plugin_stop_processing,
	.reset = Plugin_reset,
	.process = Plugin_process,
	.get_extension = Plugin_get_extension,
	.on_main_thread = Plugin_main_thread_tick,
	};

Plugin* new_Plugin(const clap_host_t* host)
{
	Plugin* plugin = (Plugin*) malloc(sizeof(Plugin));
	memset(plugin, 0, sizeof(Plugin));
	plugin->clap_plugin = clap_plugin_template;
	plugin->clap_plugin.plugin_data = plugin;
	plugin->host = host;
	return plugin;
}

void free_Plugin(Plugin* plugin)
{
	free(plugin);
}


static uint32_t Plugin_note_ports_count(const clap_plugin_t* clap_plugin, bool is_input)
{
	return is_input ? 1 : 0;
}

static bool Plugin_get_note_port(const clap_plugin_t* clap_plugin, uint32_t index, bool is_input, clap_note_port_info_t* info)
{
	if (!is_input || index != 0)
		return false;
	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_CLAP;
	info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
	snprintf(info->name, sizeof(info->name), "%s", "Note Port");
	return true;
}

static uint32_t Plugin_audio_ports_count(const clap_plugin_t* clap_plugin, bool is_input)
{
	return is_input ? 0 : 1;
}

static bool Plugin_get_audio_port(const clap_plugin_t* clap_plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info)
{
	if (is_input || index != 0)
		return false;
	info->id = 0;
	info->channel_count = 2;
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->port_type = CLAP_PORT_STEREO;
	info->in_place_pair = CLAP_INVALID_ID;
	snprintf(info->name, sizeof(info->name), "%s", "Audio Output");
	return true;
}


static uint32_t Plugin_params_count(const clap_plugin_t* clap_plugin)
{
	return NUM_PARAMS;
}

static bool Plugin_get_param_info(const clap_plugin_t* clap_plugin, uint32_t index, clap_param_info_t* info_out)
{
	if (index >= NUM_PARAMS)
		return false;
	*info_out = param_info[index];
	return true;
}

static bool Plugin_get_param_value(const clap_plugin_t* clap_plugin, clap_id id, double* value_out)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	if (id >= NUM_PARAMS)
		return false;
	*value_out = self->params[id];
	return true;
}

static bool Plugin_param_to_text(const clap_plugin_t* clap_plugin, clap_id id, double value, char* str_out, uint32_t size)
{
	if (id >= NUM_PARAMS)
		return false;
	switch (id) {
		case ATTACK_PARAM:
		case DECAY_PARAM:
			snprintf(str_out, size, "%f s", value);
			break;
		case SUSTAINS_PARAM:
			snprintf(str_out, size, value == 0.0 ? "false" : "true");
			break;
		default:
			snprintf(str_out, size, "%f", value);
			break;
		}
	return true;
}

static bool Plugin_param_text_to_value(const clap_plugin_t* clap_plugin, clap_id id, const char* str, double* value_out)
{
	char* end_ptr;
	*value_out = strtod(str, &end_ptr);
	return end_ptr != str;
}

static void Plugin_send_parameter_to_host(
	const clap_plugin_t* clap_plugin,
	clap_id id,
	const clap_output_events_t* out)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	clap_event_param_value_t event = {};
	event.header.size = sizeof(event);
	event.header.time = 0;
	event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
	event.header.type = CLAP_EVENT_PARAM_VALUE;
	event.header.flags = 0;
	event.param_id = id;
	event.cookie = NULL;
	event.note_id = -1;
	event.port_index = -1;
	event.channel = -1;
	event.key = -1;
	event.value = self->params[id];
	out->try_push(out, &event.header);
}

static void Plugin_send_all_parameters_to_host(const clap_plugin_t* clap_plugin, const clap_output_events_t* out)
{
	for (int id = 0; id < NUM_PARAMS; ++id)
		Plugin_send_parameter_to_host(clap_plugin, id, out);
}

static void Plugin_flush_params(const clap_plugin_t* clap_plugin, const clap_input_events_t* events_in, const clap_output_events_t* events_out)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;

	// Since we never change parameters on our own, the only time we need to send
	// them to the host is after loading them all from the state.
	// ...Actually, not even then.
	if (self->need_to_send_params_to_host) {
		Plugin_send_all_parameters_to_host(clap_plugin, events_out);
		self->need_to_send_params_to_host = false;
		}

	// Process events.
	const uint32_t num_events = events_in->size(events_in);
	for (int i = 0; i < num_events; ++i)
		Plugin_process_event(self, events_in->get(events_in, i));
}


static bool Plugin_save_state(const clap_plugin_t* clap_plugin, const clap_ostream_t* clap_stream)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	OutStream stream;
	OutStream_init(&stream, clap_stream);

	// Write the version.
	uint32_t version = 1;
	if (!OutStream_write_uint32(&stream, version))
		return false;

	// Write the number of parameters.
	if (!OutStream_write_uint32(&stream, NUM_PARAMS))
		return false;

	// Write the params, one by one.
	for (int i = 0; i < NUM_PARAMS; ++i) {
		double param = self->params[i];
		if (!OutStream_write_double(&stream, param))
			return false;
		}

	return true;
}

static bool Plugin_load_state(const clap_plugin_t* clap_plugin, const clap_istream_t* clap_stream)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	InStream stream;
	InStream_init(&stream, clap_stream);

	// Read the version.
	uint32_t version = InStream_read_uint32(&stream);
	if (!stream.ok)
		return false;
	// TODO: Handle older or newer versions.
	(void) version;

	// Read the number of parameters.
	uint32_t num_params = InStream_read_uint32(&stream);
	if (!stream.ok)
		return false;

	// Read the params, one by one.
	for (int i = 0; i < num_params; ++i) {
		double param = InStream_read_double(&stream);
		if (!stream.ok)
			return false;
		self->params[i] = param;
		}

	return true;
}




