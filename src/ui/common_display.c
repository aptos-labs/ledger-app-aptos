/*****************************************************************************
 *   Ledger App Aptos.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include "io.h"
#include "bip32.h"
#include "format.h"

#include "display.h"
#include "settings.h"
#include "constants.h"
#include "../globals.h"
#include "../sw.h"
#include "../address.h"
#include "action/validate.h"
#include "../transaction/types.h"
#include "../common/user_format.h"

char g_bip32_path[60];
char g_tx_type[60];
char g_address[67];
char g_gas_fee[30];
char g_struct[120];
char g_function[120];
char g_amount[30];

static size_t count_leading_zeros(const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (src[i] != 0) {
            return i;
        }
    }
    return len;
}

int ui_prepare_address() {
    if (G_context.req_type != CONFIRM_ADDRESS) {
        return io_send_sw(SW_BAD_STATE);
    }

    memset(g_bip32_path, 0, sizeof(g_bip32_path));
    if (!bip32_path_format(G_context.bip32_path,
                           G_context.bip32_path_len,
                           g_bip32_path,
                           sizeof(g_bip32_path))) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    memset(g_address, 0, sizeof(g_address));
    uint8_t address[ADDRESS_LEN] = {0};
    if (!address_from_pubkey(G_context.pk_info.raw_public_key, address, sizeof(address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    if (0 > format_prefixed_hex(address, sizeof(address), g_address, sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }

    return UI_PREPARED;
}

int ui_prepare_transaction() {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    transaction_t *transaction = &G_context.tx_info.transaction;

    if (transaction->tx_variant == TX_MESSAGE) {
        return ui_display_message();
    } else if (transaction->tx_variant == TX_RAW_MESSAGE) {
        return ui_display_raw_message();
    } else if (transaction->tx_variant != TX_UNDEFINED) {
        uint64_t gas_fee_value = transaction->gas_unit_price * transaction->max_gas_amount;
        memset(g_gas_fee, 0, sizeof(g_gas_fee));
        char gas_fee[30] = {0};
        if (!format_fpu64(gas_fee, sizeof(gas_fee), gas_fee_value, 8)) {
            return io_send_sw(SW_DISPLAY_GAS_FEE_FAIL);
        }
        snprintf(g_gas_fee, sizeof(g_gas_fee), "APT %.*s", sizeof(gas_fee), gas_fee);
        PRINTF("Gas Fee: %s\n", g_gas_fee);

        if (transaction->tx_variant == TX_RAW) {
            switch (transaction->payload_variant) {
                case PAYLOAD_ENTRY_FUNCTION:
                    return ui_display_entry_function();
                case PAYLOAD_SCRIPT:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type,
                             sizeof(g_tx_type),
                             "%s [payload = SCRIPT]",
                             RAW_TRANSACTION_SALT);
                    break;
                case PAYLOAD_MULTISIG:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type,
                             sizeof(g_tx_type),
                             "%s [payload = MULTISIG]",
                             RAW_TRANSACTION_SALT);
                    break;
                default:
                    memset(g_tx_type, 0, sizeof(g_tx_type));
                    snprintf(g_tx_type,
                             sizeof(g_tx_type),
                             "%s [payload = UNKNOWN]",
                             RAW_TRANSACTION_SALT);
                    break;
            }
        } else if (transaction->tx_variant == TX_RAW_WITH_DATA) {
            memset(g_tx_type, 0, sizeof(g_tx_type));
            snprintf(g_tx_type, sizeof(g_tx_type), RAW_TRANSACTION_WITH_DATA_SALT);
        }
    } else {
        memset(g_tx_type, 0, sizeof(g_tx_type));
        snprintf(g_tx_type, sizeof(g_tx_type), "unknown data type");
    }

    return UI_PREPARED;
}

int ui_prepare_entry_function() {
    entry_function_payload_t *function = &G_context.tx_info.transaction.payload.entry_function;
    char function_module_id_address_hex[67] = {0};

    // Be sure to display at least 1 byte, even if it is zero
    size_t leading_zeros = count_leading_zeros(function->module_id.address, ADDRESS_LEN - 1);
    if (0 > format_prefixed_hex(function->module_id.address + leading_zeros,
                                ADDRESS_LEN - leading_zeros,
                                function_module_id_address_hex,
                                sizeof(function_module_id_address_hex))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    memset(g_function, 0, sizeof(g_function));
    snprintf(g_function,
             sizeof(g_function),
             "%s::%.*s::%.*s",
             function_module_id_address_hex,
             function->module_id.name.len,
             function->module_id.name.bytes,
             function->function_name.len,
             function->function_name.bytes);
    PRINTF("Function: %s\n", g_function);

    switch (function->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return ui_display_tx_aptos_account_transfer();
        case FUNC_COIN_TRANSFER:
        case FUNC_APTOS_ACCOUNT_TRANSFER_COINS:
            return ui_display_tx_coin_transfer();
        default:
            memset(g_tx_type, 0, sizeof(g_tx_type));
            snprintf(g_tx_type, sizeof(g_tx_type), "Function call");
            PRINTF("Tx Type: %s\n", g_tx_type);
            break;
    }

    return UI_PREPARED;
}

int ui_prepare_tx_aptos_account_transfer() {
    agrs_aptos_account_trasfer_t *transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.transfer;

    // For well-known functions, display the transaction type in human-readable format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "APT transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(transfer->receiver, ADDRESS_LEN, g_address, sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    snprintf(g_amount, sizeof(g_amount), "APT %.*s", sizeof(amount), amount);
    PRINTF("Amount: %s\n", g_amount);

    return UI_PREPARED;
}

int ui_prepare_tx_coin_transfer() {
    agrs_coin_trasfer_t *transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.coin_transfer;
    char transfer_ty_coin_address_hex[67] = {0};

    // For well-known functions, display the transaction type in human-readable format
    memset(g_tx_type, 0, sizeof(g_tx_type));
    snprintf(g_tx_type, sizeof(g_tx_type), "Coin transfer");
    PRINTF("Tx Type: %s\n", g_tx_type);

    // Be sure to display at least 1 byte, even if it is zero
    size_t leading_zeros = count_leading_zeros(transfer->ty_coin.address, ADDRESS_LEN - 1);
    if (0 > format_prefixed_hex(transfer->ty_coin.address + leading_zeros,
                                ADDRESS_LEN - leading_zeros,
                                transfer_ty_coin_address_hex,
                                sizeof(transfer_ty_coin_address_hex))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    memset(g_struct, 0, sizeof(g_struct));
    snprintf(g_struct,
             sizeof(g_struct),
             "%s::%.*s::%.*s",
             transfer_ty_coin_address_hex,
             transfer->ty_coin.module_name.len,
             transfer->ty_coin.module_name.bytes,
             transfer->ty_coin.name.len,
             transfer->ty_coin.name.bytes);
    PRINTF("Coin Type: %s\n", g_struct);

    memset(g_address, 0, sizeof(g_address));
    if (0 > format_prefixed_hex(transfer->receiver, ADDRESS_LEN, g_address, sizeof(g_address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    if (!format_fpu64(g_amount, sizeof(g_amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    PRINTF("Amount: %s\n", g_amount);

    return UI_PREPARED;
}
