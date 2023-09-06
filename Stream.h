#pragma once
#include "clap/stream.h"

// Wrappers over CLAP streams to handle endianness.

extern void Streams_init();


typedef struct InStream {
	const clap_istream_t* stream;
	bool ok;
	} InStream;

extern void InStream_init(InStream* self, const clap_istream_t* stream);
extern uint32_t InStream_read_uint32(InStream* self);
extern double InStream_read_double(InStream* self);

typedef struct OutStream {
	const clap_ostream_t* stream;
	bool ok;
	} OutStream;

extern void OutStream_init(OutStream* self, const clap_ostream_t* stream);
extern bool OutStream_write_uint32(OutStream* self, uint32_t value);
extern bool OutStream_write_double(OutStream* self, double value);



