#include "Plugin.h"
#include "clap/clap.h"
#include <string.h>
#include <stdio.h>


// CLAP descriptor.

const clap_plugin_descriptor_t descriptor = {
	.clap_version = CLAP_VERSION_INIT,
	.id = "net.stevefolta.sine-ping",
	.name = "Sine Ping",
	.version = "0.0.1",
	.vendor = "Steve Folta",
	.description = "Sine Ping synth",
	.features = (const char*[]) {
		CLAP_PLUGIN_FEATURE_INSTRUMENT,
		CLAP_PLUGIN_FEATURE_SYNTHESIZER,
		CLAP_PLUGIN_FEATURE_STEREO,
		NULL,
		},
	};


// CLAP factory.

static uint32_t plugin_count(const clap_plugin_factory_t* factory)
{
	return 1;
}

static const clap_plugin_descriptor_t* plugin_descriptor(const clap_plugin_factory_t* factory, uint32_t index)
{
	return index == 0 ? &descriptor : NULL;
}

static const clap_plugin_t* create_plugin(
	const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id)
{
	if (!clap_version_is_compatible(host->clap_version) || strcmp(plugin_id, descriptor.id) != 0)
		return NULL;
	return &new_Plugin(host)->clap_plugin;
}

static const clap_plugin_factory_t factory = {
	.get_plugin_count = plugin_count,
	.get_plugin_descriptor = plugin_descriptor,
	.create_plugin = create_plugin,
	};


// CLAP entry.

bool init(const char* path)
{
	return true;
}

void deinit()
{
}

const void* get_factory(const char* factory_id)
{
	if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
		return &factory;
	return NULL;
}


const clap_plugin_entry_t clap_entry = {
	.clap_version = CLAP_VERSION_INIT,
	.init = init,
	.deinit = deinit,
	.get_factory = get_factory,
	};

