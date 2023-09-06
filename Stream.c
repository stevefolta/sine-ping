#include "Stream.h"
#include <stdbool.h>
#include <stdint.h>

// Values are stored in network-endian (big-endian) format.

static bool is_little_endian;
static bool supports_doubles;


void Streams_init()
{
	int value = 1;
	is_little_endian = ((char*) &value)[0] != 0;
	supports_doubles = (sizeof(double) == 8);
}


static uint32_t swap_uint32(uint32_t value)
{
	return
		value >> 24 |
		((value >> 8) & 0xFF00) |
		(value & 0xFF00) << 8 |
		value << 24;
}

static double swap_double(double value)
{
	uint8_t* bytes = (uint8_t*) &value;
	for (int i = 0; i < sizeof(double) / 2; ++i) {
		uint8_t tmp = bytes[sizeof(double) - i - 1];
		bytes[sizeof(double) - i - 1] = bytes[i];
		bytes[i] = tmp;
		}
	return value;
}


void InStream_init(InStream* self, const clap_istream_t* stream)
{
	self->stream = stream;
	self->ok = true;
}

uint32_t InStream_read_uint32(InStream* self)
{
	uint32_t value = 0;
	int64_t bytes_read = self->stream->read(self->stream, &value, sizeof(value));
	if (bytes_read != sizeof(value))
		self->ok = false;
	if (is_little_endian)
		value = swap_uint32(value);
	return value;
}

double InStream_read_double(InStream* self)
{
	double value = 0.0;
	int64_t bytes_read = self->stream->read(self->stream, &value, sizeof(value));
	if (bytes_read != sizeof(value) || !supports_doubles)
		self->ok = false;
	if (is_little_endian)
		value = swap_double(value);
	return value;
}


void OutStream_init(OutStream* self, const clap_ostream_t* stream)
{
	self->stream = stream;
	self->ok = true;
}

bool OutStream_write_uint32(OutStream* self, uint32_t value)
{
	if (is_little_endian)
		value = swap_uint32(value);
	int64_t bytes_written = self->stream->write(self->stream, &value, sizeof(value));
	if (bytes_written != sizeof(value)) {
		self->ok = false;
		return false;
		}
	return true;
}

bool OutStream_write_double(OutStream* self, double value)
{
	if (is_little_endian)
		value = swap_double(value);
	int64_t bytes_written = self->stream->write(self->stream, &value, sizeof(value));
	if (bytes_written != sizeof(value) || !supports_doubles) {
		self->ok = false;
		return false;
		}
	return true;
}



