#include <string.h>
#include "deserialize.h"
#include "utils.h"
#include "types.h"
#include "../common/buffer.h"
#include "../bcs/init.h"
#include "../bcs/decoder.h"

parser_status_e transaction_deserialize(buffer_t *buf, transaction_t *tx) {
    if (buf->size > MAX_TX_LEN) {
        return WRONG_LENGTH_ERROR;
    }
    transaction_init(tx);

    uint8_t *prefix;
    // skip hashed prefix bytes
    if (!bcs_read_ptr_to_fixed_bytes(buf, &prefix, TX_HASHED_PREFIX_LEN)) {
        return -1;
    }

    if (memcmp(prefix, PREFIX_RAW_TX_WITH_DATA_HASHED, TX_HASHED_PREFIX_LEN) == 0) {
        tx->tx_variant = TX_RAW_WITH_DATA;
        return PARSING_OK;
    }

    if (memcmp(prefix, PREFIX_RAW_TX_HASHED, TX_HASHED_PREFIX_LEN) == 0) {
        tx->tx_variant = TX_RAW;
    }

    // read sender address
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &tx->sender, ADDRESS_LEN)) {
        return -2;
    }
    // read sequence
    if (!bcs_read_u64(buf, &tx->sequence)) {
        return -3;
    }

    const size_t buf_footer_begin = buf->size - TX_FOOTER_LEN;
    buffer_t buf_footer = {.ptr = buf->ptr, .size = buf->size, .offset = buf_footer_begin};
    // read max_gas_amount
    if (!bcs_read_u64(&buf_footer, &tx->max_gas_amount)) {
        return -4;
    }
    // read gas_unit_price
    if (!bcs_read_u64(&buf_footer, &tx->gas_unit_price)) {
        return -5;
    }
    // read expiration_timestamp_secs
    if (!bcs_read_u64(&buf_footer, &tx->expiration_timestamp_secs)) {
        return -6;
    }
    // read chain_id
    if (!bcs_read_u8(&buf_footer, &tx->chain_id)) {
        return -7;
    }

    // read payload_variant
    uint32_t payload_variant = PAYLOAD_UNDEFINDED;
    if (!bcs_read_u32_from_uleb128(buf, &payload_variant)) {
        return -8;
    }
    if (payload_variant != PAYLOAD_ENTRY_FUNCTION && payload_variant != PAYLOAD_SCRIPT) {
        return -9;
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
            return PARSING_OK;
        default:
            return -9;
    }

    return PARSING_OK;
}

parser_status_e entry_function_payload_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return -9;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    entry_function_payload_init(payload);

    // read module id address field
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) payload->module_id.address, sizeof payload->module_id.address)) {
        return -10;
    }
    // read module_id name len field
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->module_id.name.len)) {
        return -11;
    }
    //  read module_id name bytes field
    if (!bcs_read_ptr_to_fixed_bytes(buf, &payload->module_id.name.bytes, payload->module_id.name.len)) {
        return -12;
    }
    // read function_name len field
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->function_name.len)) {
        return -13;
    }
    // read function_name bytes field
    if (!bcs_read_ptr_to_fixed_bytes(buf, &payload->function_name.bytes, payload->function_name.len)) {
        return -14;
    }

    payload->known_type = determine_function_type(tx);
    switch (payload->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return aptos_account_transfer_function_deserialize(buf, tx);
        case FUNC_COIN_TRANSFER:
            return coin_transfer_function_deserialize(buf, tx);
        default:
            return PARSING_OK;
    }

    return PARSING_OK;
}

parser_status_e aptos_account_transfer_function_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return -9;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    if (payload->known_type != FUNC_APTOS_ACCOUNT_TRANSFER) {
        return -9;
    }

    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.ty_size)) {
        return -15;
    }
    if (payload->args.ty_size != 0) {
        return -16;
    }
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.args_size)) {
        return -17;
    }
    if (payload->args.args_size != 2) {
        return -18;
    }
    uint32_t receiver_len;
    if (!bcs_read_u32_from_uleb128(buf, &receiver_len)) {
        return -19;
    }
    if (receiver_len != ADDRESS_LEN) {
        return -20;
    }
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &payload->args.transfer.receiver, ADDRESS_LEN)) {
        return -21;
    }
    uint32_t amount_len;
    if (!bcs_read_u32_from_uleb128(buf, &amount_len)) {
        return -22;
    }
    if (amount_len != sizeof(uint64_t)) {
        return -23;
    }
    if (!bcs_read_u64(buf, &payload->args.transfer.amount)) {
        return -24;
    }

    return PARSING_OK;
}

parser_status_e coin_transfer_function_deserialize(buffer_t *buf, transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return -9;
    }
    entry_function_payload_t *payload = &tx->payload.entry_function;
    if (payload->known_type != FUNC_COIN_TRANSFER) {
        return -9;
    }

    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.ty_size)) {
        return -15;
    }
    if (payload->args.ty_size != 1) {
        return -16;
    }

    uint32_t ty_arg_variant = TYPE_TAG_UNDEFINDED;
    if (!bcs_read_u32_from_uleb128(buf, &ty_arg_variant)) {
        return -25;
    }
    if (ty_arg_variant != TYPE_TAG_STRUCT) {
        return -26;
    }

    agrs_coin_trasfer_t *coin_transfer = &payload->args.coin_transfer;
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &coin_transfer->ty_coin.address, ADDRESS_LEN)) {
        return -27;
    }
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.module_name.len)) {
        return -28;
    }
    if (!bcs_read_ptr_to_fixed_bytes(buf, &coin_transfer->ty_coin.module_name.bytes, coin_transfer->ty_coin.module_name.len)) {
        return -29;
    }
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.name.len)) {
        return -30;
    }
    if (!bcs_read_ptr_to_fixed_bytes(buf, &coin_transfer->ty_coin.name.bytes, coin_transfer->ty_coin.name.len)) {
        return -31;
    }
    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &coin_transfer->ty_coin.type_args_size)) {
        return -32;
    }
    if (coin_transfer->ty_coin.type_args_size != 0) {
        return -33;
    }

    if (!bcs_read_u32_from_uleb128(buf, (uint32_t *) &payload->args.args_size)) {
        return -17;
    }
    if (payload->args.args_size != 2) {
        return -18;
    }
    uint32_t receiver_len;
    if (!bcs_read_u32_from_uleb128(buf, &receiver_len)) {
        return -19;
    }
    if (receiver_len != ADDRESS_LEN) {
        return -20;
    }
    if (!bcs_read_fixed_bytes(buf, (uint8_t *) &payload->args.transfer.receiver, ADDRESS_LEN)) {
        return -21;
    }
    uint32_t amount_len;
    if (!bcs_read_u32_from_uleb128(buf, &amount_len)) {
        return -22;
    }
    if (amount_len != sizeof(uint64_t)) {
        return -23;
    }
    if (!bcs_read_u64(buf, &payload->args.transfer.amount)) {
        return -24;
    }

    return PARSING_OK;
}

entry_function_known_type_t determine_function_type(transaction_t *tx) {
    if (tx->payload_variant != PAYLOAD_ENTRY_FUNCTION) {
        return FUNC_UNKNOWN;
    }

    if (tx->payload.entry_function.module_id.address[ADDRESS_LEN - 1] == 0x01 &&
        memcmp(tx->payload.entry_function.module_id.name.bytes, "aptos_account", 13) == 0 &&
        memcmp(tx->payload.entry_function.function_name.bytes, "transfer", 8) == 0) {
        return FUNC_APTOS_ACCOUNT_TRANSFER;
    }

    if (tx->payload.entry_function.module_id.address[ADDRESS_LEN - 1] == 0x01 &&
        memcmp(tx->payload.entry_function.module_id.name.bytes, "coin", 4) == 0 &&
        memcmp(tx->payload.entry_function.function_name.bytes, "transfer", 8) == 0) {
        return FUNC_COIN_TRANSFER;
    }

    return FUNC_UNKNOWN;
}