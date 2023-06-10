#pragma once

#include "buffer.h"

#include "types.h"

/**
 * Deserialize raw transaction in structure.
 *
 * @param[in, out] buf
 *   Pointer to buffer with serialized transaction.
 * @param[out]     tx
 *   Pointer to transaction structure.
 *
 * @return PARSING_OK if success, error status otherwise.
 *
 */
parser_status_e transaction_deserialize(buffer_t *buf, transaction_t *tx);

parser_status_e tx_raw_deserialize(buffer_t *buf, transaction_t *tx);

parser_status_e tx_variant_deserialize(buffer_t *buf, transaction_t *tx);

parser_status_e entry_function_payload_deserialize(buffer_t *buf, transaction_t *tx);

parser_status_e aptos_account_transfer_function_deserialize(buffer_t *buf, transaction_t *tx);

parser_status_e coin_transfer_function_deserialize(buffer_t *buf, transaction_t *tx);

entry_function_known_type_t determine_function_type(transaction_t *tx);
