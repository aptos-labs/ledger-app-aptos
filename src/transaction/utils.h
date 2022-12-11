#pragma once

#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

#include "types.h"

/**
 * Check if memo is encoded using ASCII characters.
 *
 * @param[in] memo
 *   Pointer to input byte buffer.
 * @param[in] memo_len
 *   Length of input byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool transaction_utils_check_encoding(const uint8_t *memo, uint64_t memo_len);
