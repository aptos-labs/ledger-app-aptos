#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "utf8.h"

bool try_push_char(uint8_t *out, size_t *out_len, uint8_t ch, size_t max_len) {
    if (*out_len < max_len) {
        out[(*out_len)++] = ch;
        return true;
    }
    return false;
}

/**
 * Checks if the data is in UTF-8 format and tries to convert it to ASCII
 * Adapted from: https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
 */
int try_utf8_to_ascii(const uint8_t *in, size_t in_len, uint8_t *out, size_t max_out_len, bool *out_is_utf8) {
    if (!in) {
        return 0;
    }
    const uint8_t unknown_char = '?';
    bool is_utf8 = false;
    size_t out_len = 0;
    size_t i = 0;
    while (i < in_len) {
        if (in[i] < 0x80) {
            if (!try_push_char(out, &out_len, in[i] & 0x7f, max_out_len)) {
                return -2;
            }
            ++i;
        } else if ((in[i] & 0xe0) == 0xc0) {
            /* 110XXXXx 10xxxxxx */
            if (i + 1 >= in_len || (in[i + 1] & 0xc0) != 0x80 ||
                (in[i] & 0xfe) == 0xc0) /* overlong? */ {
                return -1;
            } else {
                if (!try_push_char(out, &out_len, unknown_char, max_out_len)) {
                    return -2;
                }
                is_utf8 = true;
                i += 2;
            }
        } else if ((in[i] & 0xf0) == 0xe0) {
            /* 1110XXXX 10Xxxxxx 10xxxxxx */
            if (i + 2 >= in_len || (in[i + 1] & 0xc0) != 0x80 || (in[i + 2] & 0xc0) != 0x80 ||
                (in[i] == 0xe0 && (in[i + 1] & 0xe0) == 0x80) || /* overlong? */
                (in[i] == 0xed && (in[i + 1] & 0xe0) == 0xa0) || /* surrogate? */
                (in[i] == 0xef && in[i + 1] == 0xbf &&
                 (in[i + 2] & 0xfe) == 0xbe)) /* U+FFFE or U+FFFF? */ {
                return -1;
            } else {
                if (!try_push_char(out, &out_len, unknown_char, max_out_len)) {
                    return -2;
                }
                is_utf8 = true;
                i += 3;
            }
        } else if ((in[i] & 0xf8) == 0xf0) {
            /* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
            if (i + 3 >= in_len || (in[i + 1] & 0xc0) != 0x80 || (in[i + 2] & 0xc0) != 0x80 ||
                (in[i + 3] & 0xc0) != 0x80 ||
                (in[i] == 0xf0 && (in[i + 1] & 0xf0) == 0x80) || /* overlong? */
                (in[i] == 0xf4 && in[i + 1] > 0x8f) || in[i] > 0xf4) /* > U+10FFFF? */ {
                return -1;
            } else {
                if (!try_push_char(out, &out_len, unknown_char, max_out_len)) {
                    return -2;
                }
                is_utf8 = true;
                i += 4;
            }
        } else {
            return -1;
        }
    }
    // out[out_len++] = '\0';

    if (out_is_utf8) {
        *out_is_utf8 = is_utf8;
    }
    return (int) out_len;
}
