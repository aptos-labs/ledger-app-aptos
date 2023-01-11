/*****************************************************************************
 *   Ledger App Boilerplate.
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

#pragma GCC diagnostic ignored "-Wformat-invalid-specifier"  // snprintf
#pragma GCC diagnostic ignored "-Wformat-extra-args"         // snprintf

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"

#include "display.h"
#include "constants.h"
#include "../globals.h"
#include "../io.h"
#include "../sw.h"
#include "../address.h"
#include "action/validate.h"
#include "../transaction/types.h"
#include "../common/bip32.h"
#include "../common/format.h"

static action_validate_cb g_validate_callback;
static char g_amount[30];
static char g_gas_fee[30];
static char g_bip32_path[60];
static char g_address[67];
static char g_function[50];
static char g_struct[250];

// Step with icon and text
UX_STEP_NOCB(ux_display_confirm_addr_step, pn, {&C_icon_eye, "Confirm Address"});
// Step with title/text for BIP32 path
UX_STEP_NOCB(ux_display_path_step,
             bnnn_paging,
             {
                 .title = "Path",
                 .text = g_bip32_path,
             });
// Step with title/text for address
UX_STEP_NOCB(ux_display_address_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = g_address,
             });
// Step with approve button
UX_STEP_CB(ux_display_approve_step,
           pb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_display_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display address and BIP32 path:
// #1 screen: eye icon + "Confirm Address"
// #2 screen: display BIP32 Path
// #3 screen: display address
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_display_pubkey_flow,
        &ux_display_confirm_addr_step,
        &ux_display_path_step,
        &ux_display_address_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

int ui_display_address() {
    if (G_context.req_type != CONFIRM_ADDRESS || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
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
    snprintf(g_address, sizeof(g_address), "0x%.*H", sizeof(address), address);

    g_validate_callback = &ui_action_validate_pubkey;

    ux_flow_init(0, ux_display_pubkey_flow, NULL);

    return 0;
}

// Step with icon and text
UX_STEP_NOCB(ux_display_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Transaction",
             });
// Step with icon and text
UX_STEP_NOCB(ux_display_review_msg_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Message",
             });
// Step with title/text for message
UX_STEP_NOCB(ux_display_msg_step,
             bnnn_paging,
             {
                 .title = "Message",
                 .text = g_struct,
             });
// Step with title/text for transaction type
UX_STEP_NOCB(ux_display_tx_type_step,
             bnnn_paging,
             {
                 .title = "Tx Type",
                 .text = g_struct,
             });
// Step with title/text for function
UX_STEP_NOCB(ux_display_function_step,
             bnnn_paging,
             {
                 .title = "Function",
                 .text = g_function,
             });
// Step with title/text for coin type
UX_STEP_NOCB(ux_display_coin_type_step,
             bnnn_paging,
             {
                 .title = "Coin Type",
                 .text = g_struct,
             });
// Step with title/text for receiver
UX_STEP_NOCB(ux_display_receiver_step,
             bnnn_paging,
             {
                 .title = "Receiver",
                 .text = g_address,
             });
// Step with title/text for amount
UX_STEP_NOCB(ux_display_amount_step,
             bnnn_paging,
             {
                 .title = "Amount",
                 .text = g_amount,
             });
// Step with title/text for gas fee
UX_STEP_NOCB(ux_display_gas_fee_step,
             bnnn_paging,
             {
                 .title = "Gas Fee",
                 .text = g_gas_fee,
             });

// FLOW to display default transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display tx type
// #3 screen : display gas fee
// #4 screen : approve button
// #5 screen : reject button
UX_FLOW(ux_display_tx_default_flow,
        &ux_display_review_step,
        &ux_display_tx_type_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display message information:
// #1 screen : eye icon + "Review Message"
// #2 screen : display message
// #3 screen : approve button
// #4 screen : reject button
UX_FLOW(ux_display_message_flow,
        &ux_display_review_msg_step,
        &ux_display_msg_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display entry_function transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display function name
// #3 screen : display gas fee
// #4 screen : approve button
// #5 screen : reject button
UX_FLOW(ux_display_tx_entry_function_flow,
        &ux_display_review_step,
        &ux_display_function_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display aptos_account_transfer transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display function name
// #3 screen : display destination address
// #4 screen : display amount
// #5 screen : display gas fee
// #6 screen : approve button
// #7 screen : reject button
UX_FLOW(ux_display_tx_aptos_account_transfer_flow,
        &ux_display_review_step,
        &ux_display_function_step,
        &ux_display_receiver_step,
        &ux_display_amount_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display coin_transfer transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display function name
// #3 screen : display coin type
// #4 screen : display destination address
// #5 screen : display amount
// #6 screen : display gas fee
// #7 screen : approve button
// #8 screen : reject button
UX_FLOW(ux_display_tx_coin_transfer_flow,
        &ux_display_review_step,
        &ux_display_function_step,
        &ux_display_coin_type_step,
        &ux_display_receiver_step,
        &ux_display_amount_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

int ui_display_transaction() {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    g_validate_callback = &ui_action_validate_transaction;

    transaction_t *transaction = &G_context.tx_info.transaction;

    uint64_t gas_fee_value = transaction->gas_unit_price * transaction->max_gas_amount;
    memset(g_gas_fee, 0, sizeof(g_gas_fee));
    char gas_fee[30] = {0};
    if (!format_fpu64(gas_fee, sizeof(gas_fee), gas_fee_value, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);  // TODO: SW_DISPLAY_GAS_FEE_FAIL
    }
    snprintf(g_gas_fee, sizeof(g_gas_fee), "APT %.*s", sizeof(gas_fee), gas_fee);
    PRINTF("Gas Fee: %s\n", g_gas_fee);

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

    ux_flow_init(0, ux_display_tx_default_flow, NULL);

    return 0;
}

int ui_display_message() {
    memset(g_struct, 0, sizeof(g_struct));
    snprintf(g_struct,
             sizeof(g_struct),
             "%.*s",
             G_context.tx_info.raw_tx_len,
             G_context.tx_info.raw_tx);
    PRINTF("Message: %s\n", g_struct);

    ux_flow_init(0, ux_display_message_flow, NULL);

    return 0;
}

int ui_display_entry_function() {
    entry_function_payload_t *function = &G_context.tx_info.transaction.payload.entry_function;

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
    PRINTF("Function: %s\n", g_function);

    switch (function->known_type) {
        case FUNC_APTOS_ACCOUNT_TRANSFER:
            return ui_display_tx_aptos_account_transfer();
        case FUNC_COIN_TRANSFER:
            return ui_display_tx_coin_transfer();
        default:
            ux_flow_init(0, ux_display_tx_entry_function_flow, NULL);
            break;
    }
    return 0;
}

int ui_display_tx_aptos_account_transfer() {
    agrs_aptos_account_trasfer_t *transfer =
        &G_context.tx_info.transaction.payload.entry_function.args.transfer;

    memset(g_address, 0, sizeof(g_address));
    snprintf(g_address, sizeof(g_address), "0x%.*H", ADDRESS_LEN, transfer->receiver);
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    char amount[30] = {0};
    if (!format_fpu64(amount, sizeof(amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    snprintf(g_amount, sizeof(g_amount), "APT %.*s", sizeof(amount), amount);
    PRINTF("Amount: %s\n", g_amount);

    ux_flow_init(0, ux_display_tx_aptos_account_transfer_flow, NULL);

    return 0;
}

int ui_display_tx_coin_transfer() {
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
    PRINTF("Coin Type: %s\n", g_struct);

    memset(g_address, 0, sizeof(g_address));
    snprintf(g_address, sizeof(g_address), "0x%.*H", ADDRESS_LEN, transfer->receiver);
    PRINTF("Receiver: %s\n", g_address);

    memset(g_amount, 0, sizeof(g_amount));
    if (!format_fpu64(g_amount, sizeof(g_amount), transfer->amount, 8)) {
        return io_send_sw(SW_DISPLAY_AMOUNT_FAIL);
    }
    PRINTF("Amount: %s\n", g_amount);

    ux_flow_init(0, ux_display_tx_coin_transfer_flow, NULL);

    return 0;
}
