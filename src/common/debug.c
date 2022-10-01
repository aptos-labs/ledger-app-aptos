#include "os.h"

#include "debug.h"
#include "format.h"

void debug_hex_print_raw(const char* msg, const uint8_t* in, size_t in_len) {
    char buf[255] = {0};
    format_hex(in, in_len, buf, sizeof(buf));
    PRINTF("[DEBUG][RAW] %s: %s\n", msg, buf);
}

void debug_hex_print_u32_numbers(const char* msg, const uint32_t* in, size_t in_len) {
    char buf[255] = {0};
    int offset = 0;
    for (size_t i = 0; i < in_len; i++) {
        format_hex((const uint8_t*) &in[i], sizeof(uint32_t), buf + offset, sizeof(buf) - offset);
        offset += sizeof(uint32_t) * 2;
        buf[offset] = 32;
        offset += 1;
    }
    PRINTF("[DEBUG][U32] %s: %s\n", msg, buf);
}