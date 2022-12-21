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
