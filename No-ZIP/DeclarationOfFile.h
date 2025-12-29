#pragma once
#include <stdint.h>

typedef struct {
	uint64_t compressSize;
	uint64_t unCompressSize;
	char fileName[260];
} FileSpecification;