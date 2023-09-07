#pragma once

#include "clap/clap.h"

// Parameter IDs.
// These must never be reordered or removed (but a parameter can be hidden).

enum {
	ATTACK_PARAM,
	DECAY_PARAM,
	SUSTAINS_PARAM,
	TET_PARAM,

	NUM_PARAMS,
	};

// param_info[] is in the order that the parameters will be presented to the
// host.
extern const clap_param_info_t param_info[NUM_PARAMS];
extern int param_indices[NUM_PARAMS]; 	// id -> index in param_info

extern void Params_init();

