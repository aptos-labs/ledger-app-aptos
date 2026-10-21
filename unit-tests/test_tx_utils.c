#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "transaction/utils.h"
#include "transaction/types.h"

static void test_transaction_utils_check_encoding(void **state) {
    (void) state;

    const uint8_t good_ascii[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x21};  // Hello!
    const uint8_t bad_ascii[] = {0x32, 0xc3, 0x97, 0x32, 0x3d, 0x34};   // 2×2=4

    assert_true(transaction_utils_check_encoding(good_ascii, sizeof(good_ascii)));
    assert_false(transaction_utils_check_encoding(bad_ascii, sizeof(bad_ascii)));
}

static void test_transaction_utils_bcs_cmp_bytes(void **state) {
    (void) state;

    const uint8_t good_bytes[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};  // Hello
    const fixed_bytes_t bcs_bytes = {.len = 5, .bytes = (uint8_t *) good_bytes};
    const char good_str[] = "Hello";
    const char bad_str[] = "Hello!";
    assert_true(bcs_cmp_bytes(&bcs_bytes, good_str, strlen(good_str)));
    assert_false(bcs_cmp_bytes(&bcs_bytes, bad_str, strlen(bad_str)));
}

static void test_transaction_utils_strcasecmp(void **state) {
    (void) state;

    const char capital_str[] = "Hello";
    const char normal_str[] = "hello";

    assert_int_equal(_strcasecmp(capital_str, capital_str), 0);
    assert_int_equal(_strcasecmp(capital_str, normal_str), 0);
    assert_int_equal(_strcasecmp(normal_str, capital_str), 0);
    assert_int_equal(_strcasecmp(normal_str, normal_str), 0);
    assert_int_not_equal(_strcasecmp(capital_str, "World"), 0);
    assert_int_not_equal(_strcasecmp("Hello World! Hello World!", "Hello World!"), 0);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_transaction_utils_check_encoding),
                                       cmocka_unit_test(test_transaction_utils_bcs_cmp_bytes),
                                       cmocka_unit_test(test_transaction_utils_strcasecmp)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
