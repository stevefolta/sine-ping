#include "Parameters.h"

#define DEFAULT_FLAGS (CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID)

const clap_param_info_t param_info[NUM_PARAMS] = {
	{
		.id = ATTACK_PARAM,
		.name = "Attack",
		.flags = DEFAULT_FLAGS,
		.min_value = 0.0,
		.max_value = 2.0,
		.default_value = 0.01,
		},
	{
		.id = DECAY_PARAM,
		.name = "Decay",
		.flags = DEFAULT_FLAGS,
		.min_value = 0.0,
		.max_value = 2.0,
		.default_value = 0.2,
		},
};

int param_indices[NUM_PARAMS]; 	// id -> index in param_info

void Params_init()
{
	for (int i = 0; i < NUM_PARAMS; ++i)
		param_indices[param_info[i].id] = i;
}



