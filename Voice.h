#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef struct Voice {
	int state;
	float phase;
	int16_t note_id, channel, key;
	} Voice;

extern void Voice_init(Voice* self);
extern void Voice_reset(Voice* self);
extern bool Voice_is_free(Voice* self);
extern void Voice_start_note(Voice* self, int16_t note_id, int16_t channel, int16_t key);
extern bool Voice_is_for_note(Voice* self, int16_t note_id, int16_t channel, int16_t key);
extern void Voice_end_note(Voice* self);
extern void Voice_choke_note(Voice* self);
extern void Voice_render(Voice* self, uint32_t num_frames, float* l_out, float* r_out, float sample_rate);
extern bool Voice_just_ended(Voice* self);
