#include "Parameters.h"

#define DEFAULT_FLAGS (CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID)

const clap_param_info_t param_info[NUM_PARAMS] = {
	{
		.id = ATTACK_PARAM,
		.name = "Attack",
		.flags = DEFAULT_FLAGS,
		.min_value = 0.0,
		.max_value = 2.0,
		.default_value = 0.001,
		},
	{
		.id = DECAY_PARAM,
		.name = "Decay",
		.flags = DEFAULT_FLAGS,
		.min_value = 0.0,
		.max_value = 2.0,
		.default_value = 0.35,
		},
	{
		.id = SUSTAINS_PARAM,
		.name = "Sustains",
		.flags = DEFAULT_FLAGS | CLAP_PARAM_IS_STEPPED,
		.min_value = 0.0,
		.max_value = 1.0,
		.default_value = 0.0,
		},
	{
		.id = TET_PARAM,
		.name = "TET",
		.flags = DEFAULT_FLAGS | CLAP_PARAM_IS_STEPPED,
			// Does it really make sense to have parameter modulation of TET?
			// Whatever, we'll allow it.
		.min_value = 3.0,
		.max_value = 128.0,
		.default_value = 12.0,
		},
	{
		.id = A_HZ_PARAM,
		.name = "A =",
		.flags = DEFAULT_FLAGS,
		.min_value = 110.0,
		.max_value = 1760.0,
		.default_value = 440.0,
		},
};

int param_indices[NUM_PARAMS]; 	// id -> index in param_info

void Params_init()
{
	for (int i = 0; i < NUM_PARAMS; ++i)
		param_indices[param_info[i].id] = i;
}



