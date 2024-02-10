#include <string.h>

#include "buffer.h"

#include "deserialize.h"
#include "utils.h"
#include "types.h"
#include "../constants.h"
#include "../bcs/init.h"
#include "../bcs/decoder.h"

parser_status_e transaction_deserialize(buffer_t *buf, transaction_t *tx) {
    if (buf->size > MAX_TRANSACTION_LEN) {
        return WRONG_LENGTH_ERROR;
    }
    transaction_init(tx);

    parser_status_e tx_variant_parsing_status = tx_variant_deserialize(buf, tx);
    if (tx_variant_parsing_status != PARSING_OK) {
        return tx_variant_parsing_status;
    }

    switch (tx->tx_variant) {
        case TX_RAW:
            return tx_raw_deserialize(buf, tx);
        case TX_RAW_WITH_DATA:
        case TX_MESSAGE:
        case TX_RAW_MESSAGE:
            // To make sure the message is a null-terminated string
            if (buf->size == MAX_TRANSACTION_LEN && buf->ptr[MAX_TRANSACTION_LEN - 1] != 0) {
                return WRONG_LENGTH_ERROR;
            }

            __attribute__((fallthrough));
        default:
            break;
    }

    return PARSING_OK;
}

parser_status_e tx_raw_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->tx_variant != TX_RAW) {
        return TX_VARIANT_UNDEFINED_ERROR;
    }

    // read sender address
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &tx->sender, ADDRESS_LEN)) {
        return SENDER_READ_ERROR;
    }
    // read sequence
    if (!bcs_read_u64(buf, &tx->sequence)) {
        return SEQUENCE_READ_ERROR;
    }

    const size_t buf_footer_begin = buf->size - TX_FOOTER_LEN;
    buffer_t buf_footer = {.ptr = buf->ptr, .size = buf->size, .offset = buf_footer_begin};
    // read max_gas_amount
    if (!bcs_read_u64(&buf_footer, &tx->max_gas_amount)) {
        return MAX_GAS_READ_ERROR;
    }
    // read gas_unit_price
    if (!bcs_read_u64(&buf_footer, &tx->gas_unit_price)) {
        return GAS_UNIT_PRICE_READ_ERROR;
    }
    // read expiration_timestamp_secs
    if (!bcs_read_u64(&buf_footer, &tx->expiration_timestamp_secs)) {
        return EXPIRATION_READ_ERROR;
    }
    // read chain_id
    if (!bcs_read_u8(&buf_footer, &tx->chain_id)) {
        return CHAIN_ID_READ_ERROR;
    }

    // read payload_variant
    uint32_t payload_variant = PAYLOAD_UNDEFINED;
    if (!bcs_read_u32_from_uleb128(buf, &payload_variant)) {
        return PAYLOAD_VARIANT_READ_ERROR;
    }
    if (payload_variant != PAYLOAD_ENTRY_FUNCTION && payload_variant != PAYLOAD_SCRIPT &&
        payload_variant != PAYLOAD_MULTISIG) {
        return PAYLOAD_UNDEFINED_ERROR;
    }
    tx->payload_variant = payload_variant;

    parser_status_e payload_parsing_status = 0;
    switch (tx->payload_variant) {
        case PAYLOAD_ENTRY_FUNCTION:
            payload_parsing_status = entry_function_payload_deserialize(buf, tx);
            if (payload_parsing_status != PARSING_OK) {
                return payload_parsing_status;
            }
            if (tx->payload.entry_function.known_type == FUNC_APTOS_ACCOUNT_TRANSFER) {
                return (buf->offset == buf_footer_begin) ? PARSING_OK : WRONG_LENGTH_ERROR;
            }
            return PARSING_OK;
        case PAYLOAD_SCRIPT:
            // TODO: implement script fields parsing
            return PARSING_OK;
        case PAYLOAD_MULTISIG:
            // TODO: implement multisig fields parsing
            return PARSING_OK;
        default:
            return PAYLOAD_UNDEFINED_ERROR;
    }

    return PARSING_OK;
}

parser_status_e tx_variant_deserialize(buffer_t *buf, transaction_t *tx) {
    if (buf->offset != 0) {
        return TX_VARIANT_READ_ERROR;
    }

    tx->tx_variant = TX_UNDEFINED;

    uint8_t *prefix;
    // read hashed prefix bytes
    if (bcs_read_ptr_to_fixed_bytes(buf, &prefix, TX_HASHED_PREFIX_LEN)) {
        if (memcmp(prefix, PREFIX_RAW_TX_WITH_DATA_HASHED, TX_HASHED_PREFIX_LEN) == 0) {
            tx->tx_variant = TX_RAW_WITH_DATA;
            return PARSING_OK;
        }

        if (memcmp(prefix, PREFIX_RAW_TX_HASHED, TX_HASHED_PREFIX_LEN) == 0) {
            tx->tx_variant = TX_RAW;
            return PARSING_OK;
        }
    }

    // Not a transaction prefix, so we reset the offer to consider the full message
    buf->offset = 0;

    // Try to display the message as UTF8 if possible
    tx->tx_variant = transaction_utils_check_encoding(buf->ptr, buf->size)
        ? TX_MESSAGE
        : TX_RAW_MESSAGE;

    return PARSING_OK;
}

parser_status_e entry_function_payload_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return PAYLOAD_UNDEFINED_ERROR;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    entry_function_payload_init(payload);

    // read module id address field
    if (!bcs_read_fixed_bytes(buf,
                              (uint8_t *) payload->module_id.address,
                              sizeof payload->module_id.address)) {
        return MODULE_ID_ADDR_READ_ERROR;
    }
    // read module_id name len field
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->module_id.name.len)) {
        return MODULE_ID_NAME_LEN_READ_ERROR;
    }
    //  read module_id name bytes field
    if (!bcs_read_ptr_to_fixed_bytes(buf,
                                     &payload->module_id.name.bytes,
                                     payload->module_id.name.len)) {
        return MODULE_ID_NAME_BYTES_READ_ERROR;
    }
    // read function_name len field
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->function_name.len)) {
        return FUNCTION_NAME_LEN_READ_ERROR;
    }
    // read function_name bytes field
    if (!bcs_read_ptr_to_fixed_bytes(buf,
                                     &payload->function_name.bytes,
                                     payload->function_name.len)) {
        return FUNCTION_NAME_BYTES_READ_ERROR;
    }

    payload->known_type = determine_function_type(tx);
    switch (payload->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return aptos_account_transfer_function_deserialize(buf, tx);
        case FUNC_COIN_TRANSFER:
        case FUNC_APTOS_ACCOUNT_TRANSFER_COINS:
            return coin_transfer_function_deserialize(buf, tx);
        default:
            return PARSING_OK;
    }

    return PARSING_OK;
}

parser_status_e aptos_account_transfer_function_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return PAYLOAD_UNDEFINED_ERROR;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    if (payload->known_type != FUNC_APTOS_ACCOUNT_TRANSFER) {
        return PAYLOAD_UNDEFINED_ERROR;
    }

    // read type args size
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.ty_size)) {
        return TYPE_ARGS_SIZE_READ_ERROR;
    }
    if (payload->args.ty_size != 0) {
        return TYPE_ARGS_SIZE_UNEXPECTED_ERROR;
    }
    // read args size
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.args_size)) {
        return ARGS_SIZE_READ_ERROR;
    }
    if (payload->args.args_size != 2) {
        return ARGS_SIZE_UNEXPECTED_ERROR;
    }
    uint32_t receiver_len;
    // read receiver address len
    if (!bcs_read_u32_from_uleb128(buf, &receiver_len)) {
        return RECEIVER_ADDR_LEN_READ_ERROR;
    }
    if (receiver_len != ADDRESS_LEN) {
        return WRONG_ADDRESS_LEN_ERROR;
    }
    // read receiver address field
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &payload->args.transfer.receiver, ADDRESS_LEN)) {
        return RECEIVER_ADDR_READ_ERROR;
    }
    uint32_t amount_len;
    // read amount len
    if (!bcs_read_u32_from_uleb128(buf, &amount_len)) {
        return AMOUNT_LEN_READ_ERROR;
    }
    if (amount_len != sizeof(uint64_t)) {
        return WRONG_AMOUNT_LEN_ERROR;
    }
    // read amount field
    if (!bcs_read_u64(buf, &payload->args.transfer.amount)) {
        return AMOUNT_READ_ERROR;
    }

    return PARSING_OK;
}

parser_status_e coin_transfer_function_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return PAYLOAD_UNDEFINED_ERROR;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    if (payload->known_type != FUNC_COIN_TRANSFER &&
        payload->known_type != FUNC_APTOS_ACCOUNT_TRANSFER_COINS) {
        return PAYLOAD_UNDEFINED_ERROR;
    }

    // read type args size field
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.ty_size)) {
        return TYPE_ARGS_SIZE_READ_ERROR;
    }
    if (payload->args.ty_size != 1) {
        return TYPE_ARGS_SIZE_UNEXPECTED_ERROR;
    }

    uint32_t ty_arg_variant = TYPE_TAG_UNDEFINED;
    // read type tag variant
    if (!bcs_read_u32_from_uleb128(buf, &ty_arg_variant)) {
        return TYPE_TAG_READ_ERROR;
    }
    if (ty_arg_variant != TYPE_TAG_STRUCT) {
        return TYPE_TAG_UNEXPECTED_ERROR;
    }

    agrs_coin_trasfer_t *coin_transfer = &payload->args.coin_transfer;
    // read coin struct address field
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &coin_transfer->ty_coin.address, ADDRESS_LEN)) {
        return STRUCT_ADDRESS_READ_ERROR;
    }
    // read coin struct module name len
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.module_name.len)) {
        return STRUCT_MODULE_LEN_READ_ERROR;
    }
    // read coin struct module name field
    if (!bcs_read_ptr_to_fixed_bytes(buf,
                                     &coin_transfer->ty_coin.module_name.bytes,
                                     coin_transfer->ty_coin.module_name.len)) {
        return STRUCT_MODULE_BYTES_READ_ERROR;
    }
    // read coin struct name len
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.name.len)) {
        return STRUCT_NAME_LEN_READ_ERROR;
    }
    // read coin struct name field
    if (!bcs_read_ptr_to_fixed_bytes(buf,
                                     &coin_transfer->ty_coin.name.bytes,
                                     coin_transfer->ty_coin.name.len)) {
        return STRUCT_NAME_BYTES_READ_ERROR;
    }
    // read coin struct args size
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.type_args_size)) {
        return STRUCT_TYPE_ARGS_SIZE_READ_ERROR;
    }
    if (coin_transfer->ty_coin.type_args_size != 0) {
        return STRUCT_TYPE_ARGS_SIZE_UNEXPECTED_ERROR;
    }

    // read args size
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.args_size)) {
        return ARGS_SIZE_READ_ERROR;
    }
    if (payload->args.args_size != 2) {
        return ARGS_SIZE_UNEXPECTED_ERROR;
    }
    uint32_t receiver_len;
    // read receiver address len
    if (!bcs_read_u32_from_uleb128(buf, &receiver_len)) {
        return RECEIVER_ADDR_LEN_READ_ERROR;
    }
    if (receiver_len != ADDRESS_LEN) {
        return WRONG_ADDRESS_LEN_ERROR;
    }
    // read receiver address field
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &payload->args.transfer.receiver, ADDRESS_LEN)) {
        return RECEIVER_ADDR_READ_ERROR;
    }
    uint32_t amount_len;
    // read amount len
    if (!bcs_read_u32_from_uleb128(buf, &amount_len)) {
        return AMOUNT_LEN_READ_ERROR;
    }
    if (amount_len != sizeof(uint64_t)) {
        return WRONG_AMOUNT_LEN_ERROR;
    }
    // read amount field
    if (!bcs_read_u64(buf, &payload->args.transfer.amount)) {
        return AMOUNT_READ_ERROR;
    }

    return PARSING_OK;
}

entry_function_known_type_t determine_function_type(transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return FUNC_UNKNOWN;
    }

    if (tx->payload.entry_function.module_id.address[ADDRESS_LEN - 1] == 0x01 &&
        bcs_cmp_bytes(&tx->payload.entry_function.module_id.name, "aptos_account", 13) &&
        bcs_cmp_bytes(&tx->payload.entry_function.function_name, "transfer", 8)) {
        return FUNC_APTOS_ACCOUNT_TRANSFER;
    }

    if (tx->payload.entry_function.module_id.address[ADDRESS_LEN - 1] == 0x01 &&
        bcs_cmp_bytes(&tx->payload.entry_function.module_id.name, "coin", 4) &&
        bcs_cmp_bytes(&tx->payload.entry_function.function_name, "transfer", 8)) {
        return FUNC_COIN_TRANSFER;
    }

    if (tx->payload.entry_function.module_id.address[ADDRESS_LEN - 1] == 0x01 &&
        bcs_cmp_bytes(&tx->payload.entry_function.module_id.name, "aptos_account", 13) &&
        bcs_cmp_bytes(&tx->payload.entry_function.function_name, "transfer_coins", 14)) {
        return FUNC_APTOS_ACCOUNT_TRANSFER_COINS;
    }

    return FUNC_UNKNOWN;
}
