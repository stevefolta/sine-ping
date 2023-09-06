#pragma once

#include "Voice.h"
#include "clap/clap.h"

#define NUM_VOICES 32

typedef struct Plugin {
	clap_plugin_t clap_plugin;
	const clap_host_t* host;
	float sample_rate;
	Voice voices[NUM_VOICES];
	} Plugin;

extern Plugin* new_Plugin(const clap_host_t* host);
extern void free_Plugin(Plugin* plugin);

