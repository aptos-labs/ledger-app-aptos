#pragma once

#include "types.h"

void type_tag_init(type_tag_t *type_tag);

void type_tag_struct_init(type_tag_struct_t *type_tag_struct);

void fixed_bytes_init(fixed_bytes_t *fixed_bytes);

void module_id_init(module_id_t *module_id);

void entry_function_payload_init(entry_function_payload_t *payload);

void script_payload_init(script_payload_t *payload);

void transaction_init(aptos_transaction_t *tx);
