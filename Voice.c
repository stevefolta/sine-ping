#include "Voice.h"
#include <math.h>
#include <stdio.h>

enum {
	IDLE, PLAYING, DECAY, ENDED,
	};


void Voice_init(Voice* self)
{
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
	self->state = PLAYING;
	self->note_id = note_id;
	self->channel = channel;
	self->key = key;
	self->phase = 0.0;
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
	self->state = DECAY;
	self->gain = 1.0;
}


void Voice_choke_note(Voice* self)
{
	self->state = IDLE;
}


void Voice_render(Voice* self, uint32_t num_frames, float* l_out, float* r_out, float sample_rate)
{
	if (self->state == IDLE || self->state == ENDED)
		return;

	float phase_increment = 440.0 * exp2f((self->key - 57) / 12.0) / sample_rate;
	for (; num_frames > 0; --num_frames) {
		float sample = sinf(self->phase * 2.0f * 3.14159f) * 0.2f;
		if (self->state == DECAY) {
			sample *= self->gain;
			self->gain -= 0.0002;
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



