#pragma once

#include <stdint.h>
#include <stddef.h>

// TODO: move to "../transaction/types.h"
// Maximum length allowed for sequence (vectors, bytes, strings) and maps
#define MAX_SEQUENCE_LENGTH ((1ull << 31) - 1)
// Maximum number of nested structs and enum variants
#define MAX_CONTAINER_DEPTH 500
// Address size
#define ADDRESS_SIZE 32
// default coin module
#define APTOS_COIN "0x1::aptos_coin::AptosCoin"
// prefix for RawTransaction
#define RAW_TRANSACTION_SALT "APTOS::RawTransaction"
// prefix for MultiAgentRawTransaction
#define RAW_TRANSACTION_WITH_DATA_SALT "APTOS::RawTransactionWithData"
// sha3-256 hash of the RAW_TRANSACTION_SALT
uint8_t RawTxHashedPrefix[32] = {181, 233, 125, 176, 127, 160, 189, 14, 85, 152, 170, 54, 67, 169, 188, 111, 102, 147,
                                 189, 220, 26, 159, 236, 158, 103, 74, 70, 30, 170, 0, 177, 147};
// sha3-256 hash of the RAW_TRANSACTION_WITH_DATA_SALT
uint8_t RawTxWithDataHashedPrefix[32] = {94, 250, 60, 79, 2, 248, 58, 15, 75, 45, 105, 252, 149, 198, 7, 204, 2, 130,
                                         92, 196, 231, 190, 83, 110, 240, 153, 45, 240, 80, 217, 230, 124};

typedef struct {
    uint64_t high;
    uint64_t low;
} uint128_t;

typedef struct {
    int64_t high;
    uint64_t low;
} int128_t;

typedef struct {
    uint8_t *bytes;
    size_t len;
} fixed_bytes_t;

typedef enum {
    TYPE_TAG_BOOL = 0,
    TYPE_TAG_U8 = 1,
    TYPE_TAG_U64 = 2,
    TYPE_TAG_U128 = 3,
    TYPE_TAG_ADDRESS = 4,
    TYPE_TAG_SIGNER = 5,
    TYPE_TAG_VECTOR = 6,
    TYPE_TAG_STRUCT = 7
} type_tag_variant_t;

typedef struct {
    type_tag_variant_t type_tag;
    size_t size;
    void *value;
} type_tag_t;

typedef struct {
    uint8_t address[ADDRESS_SIZE];
    fixed_bytes_t module_name;
    fixed_bytes_t name;
    size_t type_args_size;
    type_tag_t *type_args;
} type_tag_struct_t;

typedef struct {
    uint8_t address[ADDRESS_SIZE];
    fixed_bytes_t name;
} module_id_t;

typedef enum {
    SCRIPT = 0,
    ENTRY_FUNCTION = 2
} payload_variant_t;

typedef struct {
    module_id_t module_id;
    fixed_bytes_t function_name;
    size_t ty_size;
    type_tag_t *ty_args;
    size_t args_size;
    fixed_bytes_t *args;
} entry_function_payload_t;

typedef struct {
    fixed_bytes_t code;
    size_t ty_size;
    type_tag_t *ty_args;
    size_t args_size;
    fixed_bytes_t *args;
} script_payload_t;

typedef struct {
    uint8_t sender[ADDRESS_SIZE];
    uint64_t sequence;
    void *payload; // entry_function_payload_t, script_payload_t
    uint64_t max_gas_amount;
    uint64_t gas_unit_price;
    uint64_t expiration_timestamp_secs;
    uint8_t chain_id;
} transaction_t;
