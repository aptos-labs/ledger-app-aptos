#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "bcs/types.h"
#include "bcs/init.h"
#include "bcs/decoder.h"
#include "bcs/utf8.h"

static void test_u8(void **state) {
    (void) state;

    uint8_t raw[] = {0xff};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t result;
    bcs_read_u8(&buf, &result);

    assert_int_equal(result, 255);
}

static void test_u32_from_uleb128(void **state) {
    (void) state;

    uint8_t raw[] = {0xcd, 0xea, 0xec, 0x31};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint32_t result;
    bcs_read_u32_from_uleb128(&buf, &result);

    assert_int_equal(result, 104543565);
}

static void test_dynamic_bytes(void **state) {
    (void) state;

    uint8_t raw[] = {6, 0x41, 0x70, 0x74, 0x6f, 0x73, 0x41, 0x70, 0x74, 0x6f, 0x73};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t result[10] = {0};
    size_t len = 0;
    bcs_read_dynamic_bytes(&buf, (uint8_t *) &result, sizeof result, &len);

    assert_memory_equal(result, ((uint8_t[]){0x41, 0x70, 0x74, 0x6f, 0x73, 0x41}), 6);
}

static void test_string(void **state) {
    (void) state;

    uint8_t raw[] = {19, 48, 120, 49, 58, 58, 99, 111, 105, 110, 58, 58, 116, 114, 97, 110, 115, 102, 101, 114};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t str[32] = {0};
    size_t str_len = 0;
    bcs_read_string(&buf, (uint8_t *) &str, sizeof str, &str_len);

    assert_string_equal(str, "0x1::coin::transfer");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_u8),
        cmocka_unit_test(test_u32_from_uleb128),
        cmocka_unit_test(test_dynamic_bytes),
        cmocka_unit_test(test_string),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
