/*****************************************************************************
 *   Ledger App Aptos.
 *   (c) 2022 Aptos Labs.
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

#ifdef HAVE_NBGL

#pragma GCC diagnostic ignored "-Wformat-invalid-specifier"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <stdbool.h>
#include <string.h>

#include "os.h"
#include "glyphs.h"
#include "nbgl_use_case.h"

#include "display.h"
#include "constants.h"
#include "../globals.h"
#include "../io.h"
#include "../sw.h"
#include "action/validate.h"
#include "../transaction/types.h"
#include "../common/format.h"
#include "menu.h"

#define MAX_TAG_VALUE_PAIRS 6

static char g_amount[30];
static char g_gas_fee[30];
static char g_address[67];
static char g_function[50];
static char g_struct[250];

static nbgl_contentTagValue_t g_pairs[MAX_TAG_VALUE_PAIRS];
static nbgl_contentTagValueList_t g_pair_list;

static void review_tx_choice(bool confirm) {
    ui_action_validate_transaction(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, ui_menu_main);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_menu_main);
    }
}

static void review_msg_choice(bool confirm) {
    ui_action_validate_transaction(confirm);
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_menu_main);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_menu_main);
    }
}

static void start_review(nbgl_operationType_t type,
                          int pair_count,
                          const char *title,
                          nbgl_choiceCallback_t callback) {
    g_pair_list.nbMaxLinesForValue = 0;
    g_pair_list.nbPairs = pair_count;
    g_pair_list.pairs = g_pairs;
    g_pair_list.wrapping = true;

    nbgl_useCaseReview(type,
                       &g_pair_list,
                       &C_aptos_logo_64px,
                       title,
                       NULL,
                       NULL,
                       callback);
}

static int format_gas_fee(transaction_t *transaction) {
    uint64_t gas_fee_value = transaction->gas_unit_price * transaction->max_gas_amount;
    memset(g_gas_fee, 0, sizeof(g_gas_fee));
    char gas_fee[30] = {0};
    if (!format_fpu64(gas_fee, sizeof(gas_fee), gas_fee_value, 8)) {
        return -1;
    }
    snprintf(g_gas_fee, sizeof(g_gas_fee), "APT %.*s", sizeof(gas_fee), gas_fee);
    return 0;
}

static int format_entry_function(entry_function_payload_t *function) {
    memset(g_function, 0, sizeof(g_function));
    snprintf(g_function,
             sizeof(g_function),
             "0x%.*H::%.*s::%.*s",
             UI_MODULE_ADDRESS_LEN,
             function->module_id.address + ADDRESS_LEN - UI_MODULE_ADDRESS_LEN,
             function->module_id.name.len,
             function->module_id.name.bytes,
             function->function_name.len,
             function->function_name.bytes);
    return 0;
}

int ui_display_transaction(void) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    transaction_t *transaction = &G_context.tx_info.transaction;

    if (format_gas_fee(transaction) < 0) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }

    if (transaction->tx_variant == TX_RAW) {
        switch (transaction->payload_variant) {
            case PAYLOAD_ENTRY_FUNCTION:
                return ui_display_entry_function();
            case PAYLOAD_SCRIPT:
                memset(g_struct, 0, sizeof(g_struct));
                snprintf(g_struct, sizeof(g_struct), "%s [payload = SCRIPT]", RAW_TRANSACTION_SALT);
                break;
            default:
                memset(g_struct, 0, sizeof(g_struct));
                snprintf(g_struct,
                         sizeof(g_struct),
                         "%s [payload = UNKNOWN]",
                         RAW_TRANSACTION_SALT);
                break;
        }
    } else if (transaction->tx_variant == TX_MESSAGE) {
        return ui_display_message();
    } else if (transaction->tx_variant == TX_RAW_WITH_DATA) {
        memset(g_struct, 0, sizeof(g_struct));
        snprintf(g_struct, sizeof(g_struct), RAW_TRANSACTION_WITH_DATA_SALT);
    } else {
        memset(g_struct, 0, sizeof(g_struct));
        snprintf(g_struct, sizeof(g_struct), "unknown data type");
    }

    // Default transaction flow: tx type + gas fee
    int idx = 0;
    g_pairs[idx].item = "Tx Type";
    g_pairs[idx].value = g_struct;
    idx++;
    g_pairs[idx].item = "Gas Fee";
    g_pairs[idx].value = g_gas_fee;
    idx++;

    start_review(TYPE_TRANSACTION, idx, "Review\ntransaction", review_tx_choice);
    return 0;
}

int ui_display_message(void) {
    memset(g_struct, 0, sizeof(g_struct));
    snprintf(g_struct,
             sizeof(g_struct),
             "%.*s",
             G_context.tx_info.raw_tx_len,
             G_context.tx_info.raw_tx);

    int idx = 0;
    g_pairs[idx].item = "Message";
    g_pairs[idx].value = g_struct;
    idx++;

    start_review(TYPE_MESSAGE, idx, "Review\nmessage", review_msg_choice);
    return 0;
}

int ui_display_entry_function(void) {
    entry_function_payload_t *function = &G_context.tx_info.transaction.payload.entry_function;
    format_entry_function(function);

    switch (function->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return ui_display_tx_aptos_account_transfer();
        case FUNC_COIN_TRANSFER:
            return ui_display_tx_coin_transfer();
        default:
            break;
    }

    // Generic entry function: function + gas fee
    int idx = 0;
    g_pairs[idx].item = "Function";
    g_pairs[idx].value = g_function;
    idx++;
    g_pairs[idx].item = "Gas Fee";
    g_pairs[idx].value = g_gas_fee;
    idx++;

    start_review(TYPE_TRANSACTION, idx, "Review\ntransaction", review_tx_choice);
    return 0;
}

int ui_display_tx_aptos_account_transfer(void) {
    agrs_aptos_account_trasfer_t *transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.transfer;

    memset(g_address, 0, sizeof(g_address));
    snprintf(g_address, sizeof(g_address), "0x%.*H", ADDRESS_LEN, transfer->receiver);

    memset(g_amount, 0, sizeof(g_amount));
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    snprintf(g_amount, sizeof(g_amount), "APT %.*s", sizeof(amount), amount);

    int idx = 0;
    g_pairs[idx].item = "Function";
    g_pairs[idx].value = g_function;
    idx++;
    g_pairs[idx].item = "Receiver";
    g_pairs[idx].value = g_address;
    idx++;
    g_pairs[idx].item = "Amount";
    g_pairs[idx].value = g_amount;
    idx++;
    g_pairs[idx].item = "Gas Fee";
    g_pairs[idx].value = g_gas_fee;
    idx++;

    start_review(TYPE_TRANSACTION, idx, "Review\ntransaction", review_tx_choice);
    return 0;
}

int ui_display_tx_coin_transfer(void) {
    agrs_coin_trasfer_t *transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.coin_transfer;

    memset(g_struct, 0, sizeof(g_struct));
    snprintf(g_struct,
             sizeof(g_struct),
             "0x%.*H..%.*H::%.*s::%.*s",
             UI_MODULE_ADDRESS_LEN,
             transfer->ty_coin.address,
             UI_MODULE_ADDRESS_LEN,
             transfer->ty_coin.address + ADDRESS_LEN - UI_MODULE_ADDRESS_LEN,
             transfer->ty_coin.module_name.len,
             transfer->ty_coin.module_name.bytes,
             transfer->ty_coin.name.len,
             transfer->ty_coin.name.bytes);

    memset(g_address, 0, sizeof(g_address));
    snprintf(g_address, sizeof(g_address), "0x%.*H", ADDRESS_LEN, transfer->receiver);

    memset(g_amount, 0, sizeof(g_amount));
    if (!format_fpu64(g_amount, sizeof(g_amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }

    int idx = 0;
    g_pairs[idx].item = "Function";
    g_pairs[idx].value = g_function;
    idx++;
    g_pairs[idx].item = "Coin Type";
    g_pairs[idx].value = g_struct;
    idx++;
    g_pairs[idx].item = "Receiver";
    g_pairs[idx].value = g_address;
    idx++;
    g_pairs[idx].item = "Amount";
    g_pairs[idx].value = g_amount;
    idx++;
    g_pairs[idx].item = "Gas Fee";
    g_pairs[idx].value = g_gas_fee;
    idx++;

    start_review(TYPE_TRANSACTION, idx, "Review\ntransaction", review_tx_choice);
    return 0;
}

#endif  // HAVE_NBGL
