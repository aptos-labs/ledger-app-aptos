#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

int try_utf8_to_ascii(const uint8_t *in,
                      size_t in_len,
                      uint8_t *out,
                      size_t max_out_len,
                      bool *out_is_utf8);
