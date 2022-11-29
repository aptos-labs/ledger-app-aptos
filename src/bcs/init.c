#include <string.h>

#include "init.h"

void type_tag_init(type_tag_t *type_tag) {
    type_tag->type_tag = 0;
    type_tag->size = 0;
    type_tag->value = NULL;
}

void type_tag_struct_init(type_tag_struct_t *type_tag_struct) {
    memset(&type_tag_struct->address, 0, ADDRESS_SIZE);
    fixed_bytes_init(&type_tag_struct->module_name);
    fixed_bytes_init(&type_tag_struct->name);
    type_tag_struct->type_args_size = 0;
    type_tag_struct->type_args = NULL;
}

void fixed_bytes_init(fixed_bytes_t *fixed_bytes) {
    fixed_bytes->bytes = NULL;
    fixed_bytes->len = 0;
}

void module_id_init(module_id_t *module_id) {
    memset(module_id->address, 0, ADDRESS_SIZE);
    fixed_bytes_init(&module_id->name);
}

void entry_function_payload_init(entry_function_payload_t *payload) {
    module_id_init(&payload->module_id);
    fixed_bytes_init(&payload->function_name);
    payload->ty_size = 0;
    payload->ty_args = NULL;
    payload->args_size = 0;
    payload->args = NULL;
}

void script_payload_init(script_payload_t *payload) {
    fixed_bytes_init(&payload->code);
    payload->ty_size = 0;
    payload->ty_args = NULL;
    payload->args_size = 0;
    payload->args = NULL;
}

void transaction_init(transaction_t *tx) {
    memset(tx->sender, 0, ADDRESS_SIZE);
    tx->sequence = 0;
    tx->payload = NULL;
    tx->max_gas_amount = 0;
    tx->gas_unit_price = 0;
    tx->expiration_timestamp_secs = 0;
    tx->chain_id = 0;
}
