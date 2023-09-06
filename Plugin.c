#include "Plugin.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static bool Plugin_init(const clap_plugin_t* clap_plugin)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	for (int i = 0; i < NUM_VOICES; ++i)
		Voice_init(&self->voices[i]);
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
		Voice_render(&self->voices[i], num_frames, l_out, r_out, self->sample_rate);
}

static void Plugin_process_event(Plugin* self, const clap_event_header_t* event)
{
	if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
		const clap_event_note_t* note_event;
		switch (event->type) {
			case CLAP_EVENT_NOTE_ON:
				{
				note_event = (clap_event_note_t*) event;
				for (int i = 0; i < NUM_VOICES; ++i) {
					if (Voice_is_free(&self->voices[i])) {
						Voice_start_note(&self->voices[i], note_event->note_id, note_event->channel, note_event->key);
						break;
						}
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
			}
		}
}

static clap_process_status Plugin_process(
	const clap_plugin_t* clap_plugin, 
	const clap_process_t* process)
{
	Plugin* self = (Plugin*) clap_plugin->plugin_data;
	const uint32_t num_frames = process->frames_count;
	const uint32_t num_events = process->in_events->size(process->in_events);
	uint32_t cur_event = 0;
	uint32_t next_event_frame = num_events > 0 ? 0 : num_frames;

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

	if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0)
		return &note_ports_extension;
	if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
		return &audio_ports_extension;
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





