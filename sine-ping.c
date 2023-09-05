#include "clap/clap.h"
#include <math.h>
#include <string.h>


// CLAP descriptor.

static const clap_plugin_descriptor_t descriptor = {
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
	/***/
	return NULL;
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

