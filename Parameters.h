#pragma once

#include "clap/clap.h"

enum {
	ATTACK_PARAM,
	DECAY_PARAM,

	NUM_PARAMS,
	};

extern const clap_param_info_t param_info[NUM_PARAMS];
extern int param_indices[NUM_PARAMS]; 	// id -> index in param_info

extern void Params_init();

