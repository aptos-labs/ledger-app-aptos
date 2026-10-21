#include "os.h"

#ifndef PARSE_H
#define PARSE_H

/**
 * Generates a string representation of a 64-bit unsigned integer, with a given number of decimals.
 * Used for GET_PRINTABLE_AMOUNT command for swap.
 *
 * @param[in] amount
 * Amount of coin.
 *
 * @param[in] decimals
 * Number of decimal places that the string should have.
 *
 * @param[out] out
 * String to save the printable amount.
 *
 * @param[out] out_len
 * Size of the buffer of out.
 *
 * @return 0 if it fails, resulting string length otherwise.
 *
 */
unsigned short print_amount(uint64_t amount, uint8_t decimals, char *out, uint32_t out_len);

/**
 * Reads exactly 2*n lowercase hecadecimal characters, storing them in exactly n bytes in `out` (1
 * byte every two hex characters); returns -1 if any character is not hexadecimal, or if less strlen
 * is less then 2*n.
 *
 * @param[in] str
 * String with the hexadecimal characters to be read (if str has "0x" prefix, pointer should be set
 * to next characters).
 *
 * @param[out] out
 * Buffer to store the bytes read. Should be at least n bytes long.
 *
 * @return 0 if success,-1 otherwise.
 *
 */
int hex_str_to_u8(const char *str, uint8_t *out, size_t n);

#endif
