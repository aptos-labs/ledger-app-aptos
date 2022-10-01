#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // int*_t, uint*_t

void debug_hex_print_raw(const char* msg, const uint8_t* in, size_t in_len);

void debug_hex_print_u32_numbers(const char* msg, const uint32_t* in, size_t in_len);