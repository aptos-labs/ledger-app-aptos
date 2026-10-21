/*******************************************************************************
 *   Ledger App Aptos.
 *   (c) 2025 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include "parse.h"

#include <string.h>

#define MAX_AMOUNT_STR_LEN 21  // 19 for u64 + 1 for '\0' +1 for '.'

/**
 * Adjusts the number of decimals of a string representation of a number.
 * If the number of decimals is greater than the number of decimals in the
 * string, it adds zeros at the end of the string.
 *
 * @param[in] src
 * The string representation of the number.
 *
 * @param[in] src_length
 * The length of the string.
 *
 * @param[in] decimals
 * The number of decimals that the string should have.
 *
 * @param[out] target
 * The string to save the adjusted number.
 *
 * @param[in] target_length
 * The size of the buffer of target.
 *
 * @return true if success, false otherwise.
 */
bool adjust_decimals(const char* src, uint32_t src_length, uint8_t decimals,
                     char* target, uint32_t target_length) {
    uint32_t start_offset;
    uint32_t last_zero_offset = 0;
    uint32_t offset = 0;

    if (src_length <= decimals) {
        uint32_t delta = decimals - src_length;
        if (target_length < src_length + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        start_offset = offset;
        for (uint32_t i = 0; i < src_length; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = src_length - decimals;
        if (target_length < src_length + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        start_offset = offset;
        while (sourceOffset < src_length) {
            target[offset++] = src[sourceOffset++];
        }
        target[offset] = '\0';
    }
    for (uint32_t i = start_offset; i < offset; i++) {
        if (target[i] == '0') {
            if (last_zero_offset == 0) {
                last_zero_offset = i;
            }
        } else {
            last_zero_offset = 0;
        }
    }
    if (last_zero_offset != 0) {
        target[last_zero_offset] = '\0';
        if (target[last_zero_offset - 1] == '.') {
            target[last_zero_offset - 1] = '\0';
        }
    }
    return true;
}

unsigned short print_amount(uint64_t amount, uint8_t decimals, char* out,
                            uint32_t out_len) {
    if (amount == 0) {
        if (out_len < 2) {
            return 0;
        }
        out[0] = '0';
        out[1] = '\0';

        return strlen(out);
    }

    char tmp[MAX_AMOUNT_STR_LEN];
    char tmp2[MAX_AMOUNT_STR_LEN];
    uint32_t num_digits = 0, i;
    uint64_t base = 1;

    while (base <= amount) {
        base *= 10;
        num_digits++;
    }
    if (num_digits > sizeof(tmp) - 2) {
        return 0;
    }
    base /= 10;
    for (i = 0; i < num_digits; i++) {
        tmp[i] = '0' + ((amount / base) % 10);
        base /= 10;
    }
    tmp[i] = '\0';
    out[0] = '\0';
    if (adjust_decimals(tmp, i, decimals, tmp2, MAX_AMOUNT_STR_LEN)) {
        if (strlen(tmp2) < out_len - 1) {
            strlcpy(out, tmp2, out_len);
        }
    }
    return strlen(out);
}

static bool is_digit(char c) { return '0' <= c && c <= '9'; }

static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static bool is_lowercase_hex(char c) {
    return is_digit(c) || ('a' <= c && c <= 'f');
}

static bool is_uppercase(char c) {
    return is_alpha(c) && ('A' <= c && c <= 'Z');
}

static char to_lowercase(char c) {
    if (is_uppercase(c)) {
        return (c - 'A' + 'a');
    }
    return c;
}

static uint8_t lowercase_hex_to_int(char c) {
    return (uint8_t)(is_digit(c) ? c - '0' : c - 'a' + 10);
}

int hex_str_to_u8(const char* str, uint8_t* out, size_t n) {
    if (strlen(str) < 2 * n) {
        return -1;
    }
    for (unsigned int i = 0; i < n; i++) {
        char c1, c2;
        c1 = to_lowercase(str[2 * i]);
        c2 = to_lowercase(str[2 * i + 1]);
        if (!is_lowercase_hex(c1) || !is_lowercase_hex(c2)) {
            return -1;
        }
        out[i] = 16 * lowercase_hex_to_int(c1) + lowercase_hex_to_int(c2);
    }
    return 0;
}
