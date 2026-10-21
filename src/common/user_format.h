#pragma once

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int*_t, uint*_t

/**
 * Format byte buffer to uppercase hexadecimal string prefixed with '0x'.
 *
 * @param[in]  in
 *   Pointer to input byte buffer.
 * @param[in]  in_len
 *   Length of input byte buffer.
 * @param[out] out
 *   Pointer to output string.
 * @param[in]  out_len
 *   Length of output string.
 *
 * @return number of bytes written if success, -1 otherwise.
 *
 */
int format_prefixed_hex(const uint8_t *in, size_t in_len, char *out, size_t out_len);

bool is_str_interrupted(const char *src, size_t len);
