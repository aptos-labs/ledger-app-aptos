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

/**
 * Compares two strings case-insensitive.
 * NOTE: this is implemented because the SDK does not have a working strcasecmp.
 * Similar issue happens on ethereum
 * (https://github.com/LedgerHQ/app-ethereum/blob/45b96b767d017c73a14fdaccbb8947be0cd8ea6c/src_features/signTx/logic_signTx.c#L329)
 *
 * TODO(jmartins): remove this function when the SDK has a working strcasecmp.
 *
 * @param[in] s1
 *   String to compare
 *
 * @param[in] s2
 *   String to compare against
 *
 * @return 0 if the strings are equal, less or greater than if s1 is lexicographically less or
 * greater than s2
 */
int _strcasecmp(const char *s1, const char *s2);
