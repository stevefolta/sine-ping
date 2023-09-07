#include "Voice.h"
#include "Plugin.h"
#include <math.h>
#include <stdio.h>

enum {
	IDLE, ATTACK, PLAYING, DECAY, ENDED,
	};

static double Voice_param_value(Voice* self, int param_id);


void Voice_init(Voice* self, struct Plugin* plugin)
{
	self->plugin = plugin;
	self->state = IDLE;
}

void Voice_reset(Voice* self)
{
	self->state = IDLE;
}


bool Voice_is_free(Voice* self)
{
	return self->state == IDLE;
}

void Voice_start_note(Voice* self, int16_t note_id, int16_t channel, int16_t key)
{
	self->state = ATTACK;
	self->note_id = note_id;
	self->channel = channel;
	self->key = key;
	self->phase = 0.0;
	for (int i = 0; i < NUM_PARAMS; ++i)
		self->param_offsets[i] = 0.0;
}


bool Voice_is_for_note(Voice* self, int16_t note_id, int16_t channel, int16_t key)
{
	return
		(self->state != IDLE && self->state != ENDED) &&
		(key == -1 || key == self->key) &&
		(note_id == -1 || note_id == self->note_id) &&
		(channel == -1 || channel == self->channel);
}


void Voice_end_note(Voice* self)
{
	if (self->state == ATTACK || self->state == PLAYING)
		self->state = DECAY;
}


void Voice_choke_note(Voice* self)
{
	self->state = IDLE;
}


void Voice_render(Voice* self, uint32_t num_frames, float* l_out, float* r_out)
{
	if (self->state == IDLE || self->state == ENDED)
		return;

	double tet = Voice_param_value(self, TET_PARAM);
	double a_hz = Voice_param_value(self, A_HZ_PARAM);
	double phase_increment = a_hz * exp2((self->key - 57) / tet) / self->plugin->sample_rate;
	float gain_change_per_sample = 0.0;
	if (self->state == ATTACK)
		gain_change_per_sample = 1.0 / (self->plugin->sample_rate * Voice_param_value(self, ATTACK_PARAM));
	else if (self->state == DECAY)
		gain_change_per_sample = 1.0 / (self->plugin->sample_rate * Voice_param_value(self, DECAY_PARAM));
	for (; num_frames > 0; --num_frames) {
		float sample = sinf(self->phase * 2.0f * 3.14159f) * 0.05f;
		if (self->state == ATTACK) {
			sample *= self->gain;
			self->gain += gain_change_per_sample;
			if (self->gain >= 1.0) {
				self->gain = 1.0;
				if (Voice_param_value(self, SUSTAINS_PARAM) != 0)
					self->state = PLAYING;
				else {
					self->state = DECAY;
					gain_change_per_sample = 1.0 / (self->plugin->sample_rate * Voice_param_value(self, DECAY_PARAM));
					}
				}
			}
		else if (self->state == DECAY) {
			sample *= self->gain;
			self->gain -= gain_change_per_sample;
			if (self->gain < 0.0) {
				self->gain = 0.0;
				self->state = ENDED;
				break;
				}
			}
		*l_out++ += sample;
		*r_out++ += sample;
		self->phase += phase_increment;
		self->phase -= floorf(self->phase);
		}
}


bool Voice_just_ended(Voice* self)
{
	if (self->state == ENDED) {
		self->state = IDLE;
		return true;
		}
	return false;
}


static double Voice_param_value(Voice* self, int param_id)
{
	double value = self->plugin->params[param_id] + self->param_offsets[param_id];
	const clap_param_info_t* cur_param_info = &param_info[param_indices[param_id]];
	if (value < cur_param_info->min_value)
		value = cur_param_info->min_value;
	else if (value > cur_param_info->max_value)
		value = cur_param_info->max_value;
	return value;
}



