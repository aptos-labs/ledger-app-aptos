#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <cmocka.h>

#include "bcs/types.h"
#include "bcs/init.h"
#include "bcs/decoder.h"
#include "bcs/utf8.h"

int from_hex(const char *in, uint8_t *out, size_t out_len) {
    size_t len = strlen(in);
    if (len % 2 != 0) return 0;

    size_t final_len = len / 2;
    if (out_len < final_len) return 0;

    const char *offset = in;
    for (size_t i = 0; i < final_len; i++) {
        sscanf(offset, "%2hhx", &out[i]);
        offset += 2;
    }
    return final_len;
}

void print_hex(const char *msg, uint8_t *in, size_t len) {
    printf("%s 0x", msg);
    for (size_t i = 0; i < len / sizeof *in; i++) printf("%02x", in[i]);
    printf("\n");
}

void test_u8() {
    uint8_t raw[] = {0xff};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t result;
    bcs_read_u8(&buf, &result);
    printf("[TEST][u8] 0x%02x\n", result);
}

void test_u32_from_uleb128() {
    uint8_t raw[] = {0xcd, 0xea, 0xec, 0x31};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint32_t result;
    bcs_read_u32_from_uleb128(&buf, &result);
    printf("[TEST][u32_from_uleb128] %d\n", result);
}

void test_dynamic_bytes() {
    uint8_t raw[] = {6, 0x41, 0x70, 0x74, 0x6f, 0x73, 0x41, 0x70, 0x74, 0x6f, 0x73};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t result[10] = {0};
    size_t len = 0;
    bcs_read_dynamic_bytes(&buf, &result, sizeof result, &len);
    print_hex("[TEST][dynamic_bytes]", result, len);
}

void test_string() {
    uint8_t raw[] = {20, 19, 48, 120, 49, 58, 58,
                     99, 111, 105, 110, 58, 58,
                     116, 114, 97, 110, 115, 102,
                     101, 114};
    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    uint8_t str[32] = {0};
    size_t str_len = 0;
    bcs_read_string(&buf, &str, sizeof str, &str_len);
    printf("[TEST][string] %ld - %s\n", str_len, str);
}

void test_utf8_to_ascii() {
    const char utf8_hex[] = "133078313a3a636f696e3a3a7472616e73666572";
    uint8_t utf8_bytes[32] = {0};
    uint8_t ascii_bytes[32] = {0};
    size_t utf8_len = from_hex(utf8_hex, utf8_bytes, sizeof utf8_bytes);
    size_t ascii_len = try_utf8_to_ascii(utf8_bytes, utf8_len, ascii_bytes, sizeof ascii_bytes, NULL);
    printf("[TEST][utf8_to_ascii] %ld - %s\n", utf8_len, ascii_bytes);
}

void test_entry_func_transaction() {
    uint8_t raw[] = {
            181, 233, 125, 176, 127, 160, 189, 14, 85, 152, 170, 54, 67, 169, 188, 111, 102, 147, 189, 220, 26, 159,
            236, 158, 103, 74, 70, 30, 170, 0, 177, 147, 134, 191, 27, 88, 148, 45, 155, 241, 36, 117, 164, 31, 47, 67,
            185, 112, 135, 221, 145, 147, 127, 64, 30, 236, 8, 49, 17, 104, 169, 186, 194, 243, 1, 0, 0, 0, 0, 0, 0, 0,
            2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, 99,
            111, 105, 110, 8, 116, 114, 97, 110, 115, 102, 101, 114, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 10, 97, 112, 116, 111, 115, 95, 99, 111, 105, 110, 9, 65,
            112, 116, 111, 115, 67, 111, 105, 110, 0, 2, 32, 167, 103, 106, 0, 59, 111, 180, 116, 72, 183, 155, 141,
            104, 210, 136, 70, 185, 41, 50, 148, 28, 146, 190, 236, 209, 159, 27, 238, 106, 104, 82, 8, 8, 205, 2, 0, 0,
            0, 0, 0, 0, 32, 78, 0, 0, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 19, 132, 101, 99, 0, 0, 0, 0, 36
    };

    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    // skip hashed prefix bytes
    buffer_seek_cur(&buf, sizeof RawTxHashedPrefix);

    // deserialized transfer structure
    transaction_t tx;
    transaction_init(&tx);

    // read sender address
    bcs_read_fixed_bytes(&buf, (uint8_t *) &tx.sender, sizeof tx.sender);

    // read sequence
    bcs_read_u64(&buf, &tx.sequence);

    // read payload type
    uint32_t payload_enum = -1; // 0 is Script Payload type
    bcs_read_u32_from_uleb128(&buf, &payload_enum);

    // read entry function structure
    entry_function_payload_t payload;
    entry_function_payload_init(&payload);

    // read module id address field
    bcs_read_fixed_bytes(&buf, (uint8_t *) &payload.module_id.address, sizeof payload.module_id.address);

    // read module id name field
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &payload.module_id.name.len);
    payload.module_id.name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * payload.module_id.name.len);
    bcs_read_fixed_bytes(&buf, payload.module_id.name.bytes, payload.module_id.name.len);

    // read function_name
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &payload.function_name.len);
    payload.function_name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * payload.function_name.len);
    bcs_read_fixed_bytes(&buf, payload.function_name.bytes, payload.function_name.len);

    // read ty_args[]
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &payload.ty_size);
    payload.ty_args = (type_tag_t *) malloc(sizeof(type_tag_t) * payload.ty_size);
    for (size_t i = 0; i < payload.ty_size; i++) {
        type_tag_t ty_val;
        type_tag_init(&ty_val);

        bcs_read_u32_from_uleb128(&buf, &ty_val.type_tag);
        if (ty_val.type_tag < TYPE_TAG_VECTOR) {
            bcs_read_type_tag_fixed(&buf, &ty_val);
        } else if (ty_val.type_tag == TYPE_TAG_VECTOR) {
            // read vector
        } else if (ty_val.type_tag == TYPE_TAG_STRUCT) {
            ty_val.size = sizeof(type_tag_struct_t);
            type_tag_struct_t ty_struct;
            type_tag_struct_init(&ty_struct);

            bcs_read_type_tag_struct(&buf, &ty_struct);
        }
        payload.ty_args[i] = ty_val;
    }

    // read args[]
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &payload.args_size);
    payload.args = (fixed_bytes_t *) malloc(sizeof(fixed_bytes_t) * payload.args_size);
    for (size_t i = 0; i < payload.args_size; i++) {
        fixed_bytes_t arg;
        fixed_bytes_init(&arg);

        bcs_read_u32_from_uleb128(&buf, (uint32_t *) &arg.len);
        arg.bytes = (uint8_t *) malloc(sizeof(uint8_t) * arg.len);
        bcs_read_fixed_bytes(&buf, arg.bytes, arg.len);
    }
    tx.payload = &payload;

    // read max_gas_amount
    bcs_read_u64(&buf, &tx.max_gas_amount);
    // read gas_unit_price
    bcs_read_u64(&buf, &tx.gas_unit_price);
    // read expiration_timestamp_secs
    bcs_read_u64(&buf, &tx.expiration_timestamp_secs);
    // read chain_id
    bcs_read_u8(&buf, &tx.chain_id);

    printf("[TEST][test_entry_func_transaction]\n");
    printf("\tsender: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", tx.sender[i]);
    }
    printf("\n");
    printf("\tsequence: %llu\n", tx.sequence);
    printf("\tpayload:\n");
    printf("\t\tmodule_id:\n");
    printf("\t\t\taddress: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", (*(entry_function_payload_t *) tx.payload).module_id.address[i]);
    }
    printf("\n");
    printf("\t\t\tname: %s\n", (*(entry_function_payload_t *) tx.payload).module_id.name.bytes);
    printf("\t\tfunction: %s\n", (*(entry_function_payload_t *) tx.payload).function_name.bytes);
    printf("\t\tty_args: %zu\n", (*(entry_function_payload_t *) tx.payload).ty_size);
    printf("\t\targs: %zu\n", (*(entry_function_payload_t *) tx.payload).args_size);
    printf("\tmax_gas_amount: %llu\n", tx.max_gas_amount);
    printf("\tgas_unit_price: %llu\n", tx.gas_unit_price);
    printf("\texpiration_timestamp_secs: %llu\n", tx.expiration_timestamp_secs);
    printf("\tchain_id: %d\n", tx.chain_id);
}

void test_transfer_tx() {
    uint8_t raw[] = {
            181, 233, 125, 176, 127, 160, 189, 14, 85, 152, 170, 54, 67, 169, 188, 111, 102, 147, 189, 220, 26, 159,
            236, 158, 103, 74, 70, 30, 170, 0, 177, 147, 134, 191, 27, 88, 148, 45, 155, 241, 36, 117, 164, 31, 47, 67,
            185, 112, 135, 221, 145, 147, 127, 64, 30, 236, 8, 49, 17, 104, 169, 186, 194, 243, 1, 0, 0, 0, 0, 0, 0, 0,
            2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, 99,
            111, 105, 110, 8, 116, 114, 97, 110, 115, 102, 101, 114, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 10, 97, 112, 116, 111, 115, 95, 99, 111, 105, 110, 9, 65,
            112, 116, 111, 115, 67, 111, 105, 110, 0, 2, 32, 167, 103, 106, 0, 59, 111, 180, 116, 72, 183, 155, 141,
            104, 210, 136, 70, 185, 41, 50, 148, 28, 146, 190, 236, 209, 159, 27, 238, 106, 104, 82, 8, 8, 205, 2, 0, 0,
            0, 0, 0, 0, 32, 78, 0, 0, 0, 0, 0, 0, 100, 0, 0, 0, 0, 0, 0, 0, 19, 132, 101, 99, 0, 0, 0, 0, 36
    };

    buffer_t buf = {.ptr = raw, .size = sizeof raw, .offset = 0};
    // skip hashed prefix bytes
    buffer_seek_cur(&buf, sizeof RawTxHashedPrefix);

    transfer_tx_t tx;
    transfer_tx_init(&tx);

    // read sender address
    bcs_read_fixed_bytes(&buf, (uint8_t *) &tx.sender, ADDRESS_SIZE);

    // read sequence
    bcs_read_u64(&buf, &tx.sequence);

    // read payload variant
    payload_variant_t payload_variant;
    bcs_read_u32_from_uleb128(&buf, &payload_variant);

    // read module id struct
    bcs_read_fixed_bytes(&buf, (uint8_t *) &tx.module.address, ADDRESS_SIZE);
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &tx.module.name.len);
    tx.module.name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * tx.module.name.len);
    bcs_read_fixed_bytes(&buf, tx.module.name.bytes, tx.module.name.len);

    // read function name
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &tx.function.len);
    tx.function.bytes = (uint8_t *) malloc(sizeof(uint8_t) * tx.function.len);
    bcs_read_fixed_bytes(&buf, tx.function.bytes, tx.function.len);

    // read ty_args[type_tag_struct]
    uint32_t ty_args_size = 0;
    bcs_read_u32_from_uleb128(&buf, &ty_args_size);
    type_tag_variant_t ty_arg_variant;
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &ty_arg_variant);
    bcs_read_fixed_bytes(&buf, (uint8_t *) &tx.ty_arg.address, ADDRESS_SIZE);
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &tx.ty_arg.module_name.len);
    tx.ty_arg.module_name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * tx.ty_arg.module_name.len);
    bcs_read_fixed_bytes(&buf, tx.ty_arg.module_name.bytes, tx.ty_arg.module_name.len);
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &tx.ty_arg.name.len);
    tx.ty_arg.name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * tx.ty_arg.name.len);
    bcs_read_fixed_bytes(&buf, tx.ty_arg.name.bytes, tx.ty_arg.name.len);
    bcs_read_u32_from_uleb128(&buf, (uint32_t *) &tx.ty_arg.type_args_size);

    // read args[receiver_address, amount]
    uint32_t args_size = 0;
    bcs_read_u32_from_uleb128(&buf, &args_size);
    uint32_t receiver_len;
    bcs_read_u32_from_uleb128(&buf, &receiver_len);
    bcs_read_fixed_bytes(&buf, (uint8_t *) &tx.receiver, ADDRESS_SIZE);
    uint32_t amount_len;
    bcs_read_u32_from_uleb128(&buf, &amount_len);
    bcs_read_u64(&buf, &tx.amount);

    // read max_gas_amount
    bcs_read_u64(&buf, &tx.max_gas_amount);
    // read gas_unit_price
    bcs_read_u64(&buf, &tx.gas_unit_price);
    // read expiration_timestamp_secs
    bcs_read_u64(&buf, &tx.expiration_timestamp_secs);
    // read chain_id
    bcs_read_u8(&buf, &tx.chain_id);

    printf("[TEST][test_transfer_tx]\n");
    printf("\tsender: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", tx.sender[i]);
    }
    printf("\n");
    printf("\tsequence: %llu\n", tx.sequence);
    printf("\tpayload:\n");
    printf("\t\tmodule: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", tx.module.address[i]);
    }
    printf("::%s::%s\n", tx.module.name.bytes, tx.function.bytes);
    printf("\t\tty_args: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", tx.ty_arg.address[i]);
    }
    printf("::%s::%s\n", tx.ty_arg.module_name.bytes, tx.ty_arg.name.bytes);
    printf("\t\treceiver: 0x");
    for (size_t i = 0; i < ADDRESS_SIZE; i++) {
        printf("%hhX", tx.receiver[i]);
    }
    printf("\n");
    printf("\t\tamount: %llu\n", tx.amount);
    printf("\tmax_gas_amount: %llu\n", tx.max_gas_amount);
    printf("\tgas_unit_price: %llu\n", tx.gas_unit_price);
    printf("\texpiration_timestamp_secs: %llu\n", tx.expiration_timestamp_secs);
    printf("\tchain_id: %d\n", tx.chain_id);
}

int main() {
    test_u8();
    test_u32_from_uleb128();
    test_dynamic_bytes();
    test_utf8_to_ascii();
    test_string();
    test_entry_func_transaction();
    test_transfer_tx();

    return 0;
}