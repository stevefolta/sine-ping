#pragma once

#include "Voice.h"
#include "Parameters.h"
#include "clap/clap.h"
#include <stdatomic.h>

#define NUM_VOICES 32

typedef struct Plugin {
	clap_plugin_t clap_plugin;
	const clap_host_t* host;
	float sample_rate;
	Voice voices[NUM_VOICES];
	_Atomic(double) params[NUM_PARAMS];
	atomic_bool need_to_send_params_to_host;
	} Plugin;

extern Plugin* new_Plugin(const clap_host_t* host);
extern void free_Plugin(Plugin* plugin);

