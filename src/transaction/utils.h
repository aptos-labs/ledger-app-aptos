#pragma once

#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

#include "types.h"

/**
 * Check if msg is encoded using ASCII characters.
 *
 * @param[in] msg
 *   Pointer to input byte buffer.
 * @param[in] msg_len
 *   Length of input byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool transaction_utils_check_encoding(const uint8_t *msg, uint64_t msg_len);

/**
 * Compares the fixed_bytes_t bcs_bytes to the memory pointed by value.
 *
 * @param[in] bcs_bytes
 *   Pointer to fixed_bytes_t struct.
 * @param[in] value
 *   Pointer to input block of memory.
 * @param[in] len
 *   Length of input bytes to compare.
 *
 * @return true if success, false otherwise.
 *
 */
bool bcs_cmp_bytes(const fixed_bytes_t *bcs_bytes, const void *value, size_t len);
