#pragma once
#include <stdint.h>
#include <stddef.h>
#include "db.h"

uint8_t *map_file(const char *filepath, size_t *out_size);
void unmap_file(uint8_t *mapped, size_t size);
uint64_t parse_stream(const uint8_t *buffer, size_t file_size, db_state_t *state);