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

#ifdef HAVE_BAGL

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include "io.h"
#include "format.h"

#include "bagl_display.h"
#include "display.h"
#include "settings.h"
#include "menu.h"
#include "constants.h"
#include "../globals.h"
#include "../sw.h"
#include "action/validate.h"
#include "../common/user_format.h"

#define DOTS "[...]"

static action_validate_cb g_validate_callback;
static action_extend_ctx_t g_allow_blind_sign_ctx;

// Validate/Invalidate public key and go back to home
static void ui_action_validate_pubkey(bool choice) {
    validate_pubkey(choice);
    ui_menu_main();
}

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_menu_main();
}

// Action to allow blind signing in settings
static void ui_action_allow_blind_signing(const ux_flow_step_t *const *steps) {
    settings_allow_blind_signing_change(1);

    // Passed UX_FLOW steps are expected to contain a blind signing warning on the first step.
    // Skip it for better UX here.
    ux_flow_init(0, steps, steps[1]);
}

#ifdef TARGET_NANOS
UX_STEP_NOCB(ux_display_blind_sign_banner_step,
             bnnn_paging,
             {
                 .title = "Error",
                 .text = "Blind signing must be enabled in Settings",
             });
#else
// Step with icon and text
UX_STEP_NOCB(ux_display_blind_sign_banner_step,
             pnn,
             {
                 &C_icon_warning,
                 "Blind signing must be",
                 "enabled in Settings",
             });
#endif
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
// Step with the button to change settings
UX_STEP_CB(ux_display_allow_blind_sign_step,
           pnn,
           { g_allow_blind_sign_ctx.call(g_allow_blind_sign_ctx.steps); },
           {
               &C_icon_validate_14,
               "Allow",
               "Blind Signing",
           });

// FLOW to display blind signing banner:
// #1 screen : warning icon + "Blind signing must be enabled in Settings"
// #2 screen : reject button
UX_FLOW(ux_display_blind_sign_banner_flow,
        &ux_display_blind_sign_banner_step,
        &ux_display_allow_blind_sign_step,
        &ux_display_reject_step);

void ui_flow_display(const ux_flow_step_t *const *steps) {
    ux_flow_init(0, steps, NULL);
}

// This function should always use UX_FLOW containing the blind signing warning on the first step!
void ui_flow_verified_display(const ux_flow_step_t *const *steps) {
    if (N_storage.settings.allow_blind_signing) {
        ui_flow_display(steps);
    } else {
        g_allow_blind_sign_ctx.call = &ui_action_allow_blind_signing;
        g_allow_blind_sign_ctx.steps = steps;
        ui_flow_display(ux_display_blind_sign_banner_flow);
    }
}

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
    g_validate_callback = &ui_action_validate_pubkey;

    const int ret = ui_prepare_address();
    if (ret == UI_PREPARED) {
        ui_flow_display(ux_display_pubkey_flow);
        return 0;
    }

    return ret;
}

// Step with icon and text
UX_STEP_NOCB(ux_display_blind_warn_step,
             pnn,
             {
                 &C_icon_warning,
                 "Blind",
                 "Signing",
             });
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
                 .text = (const char *) G_context.tx_info.raw_tx,
             });
// Step with title/text for message in short form
UX_STEP_NOCB(ux_display_short_msg_step,
             bnnn_paging,
             {
                 .title = "Message",
                 .text = g_struct,
             });
// Step with title/text for message in raw form
UX_STEP_NOCB(ux_display_raw_msg_step,
             bnnn_paging,
             {
                 .title = "Raw message",
                 .text = g_struct,
             });
// Step with title/text for transaction type
UX_STEP_NOCB(ux_display_tx_type_step,
             bnnn_paging,
             {
                 .title = "Transaction Type",
                 .text = g_tx_type,
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
// #1 screen : warning icon + "Blind Signing"
// #2 screen : eye icon + "Review Transaction"
// #3 screen : display tx type
// #4 screen : display gas fee
// #5 screen : approve button
// #6 screen : reject button
UX_FLOW(ux_display_blind_tx_default_flow,
        &ux_display_blind_warn_step,
        &ux_display_review_step,
        &ux_display_tx_type_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// SEQUENCE to display message information:
// #1 screen : eye icon + "Review Message"
// #2 screen : display message
// #3 screen : approve button
// #4 screen : reject button
#define SEQUENCE_MESSAGE                                                         \
    &ux_display_review_msg_step, &ux_display_msg_step, &ux_display_approve_step, \
        &ux_display_reject_step
UX_FLOW(ux_display_message_flow, SEQUENCE_MESSAGE);
// preceding screen : warning icon + "Blind Signing"
UX_FLOW(ux_display_blind_message_flow, &ux_display_blind_warn_step, SEQUENCE_MESSAGE);

// FLOW to display message information in short form:
// #1 screen : eye icon + "Review Message"
// #2 screen : display short message
// #3 screen : approve button
// #4 screen : reject button
#define SEQUENCE_SHORT_MESSAGE                                                         \
    &ux_display_review_msg_step, &ux_display_short_msg_step, &ux_display_approve_step, \
        &ux_display_reject_step
UX_FLOW(ux_display_short_message_flow, SEQUENCE_SHORT_MESSAGE);
// preceding screen : warning icon + "Blind Signing"
UX_FLOW(ux_display_blind_short_message_flow, &ux_display_blind_warn_step, SEQUENCE_SHORT_MESSAGE);

// FLOW to display message information in raw form:
// #1 screen : eye icon + "Review Message"
// #2 screen : display raw message
// #3 screen : approve button
// #4 screen : reject button
#define SEQUENCE_RAW_MESSAGE                                                         \
    &ux_display_review_msg_step, &ux_display_raw_msg_step, &ux_display_approve_step, \
        &ux_display_reject_step
UX_FLOW(ux_display_raw_message_flow, SEQUENCE_RAW_MESSAGE);
// preceding screen : warning icon + "Blind Signing"
UX_FLOW(ux_display_blind_raw_message_flow, &ux_display_blind_warn_step, SEQUENCE_RAW_MESSAGE);

// FLOW to display entry_function transaction information:
// #1 screen : warning icon + "Blind Signing"
// #2 screen : eye icon + "Review Transaction"
// #3 screen : display tx type
// #4 screen : display function name
// #5 screen : display gas fee
// #6 screen : approve button
// #7 screen : reject button
UX_FLOW(ux_display_blind_tx_entry_function_flow,
        &ux_display_blind_warn_step,
        &ux_display_review_step,
        &ux_display_tx_type_step,
        &ux_display_function_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display aptos_account_transfer transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display tx type
// #3 screen : display function name
// #4 screen : display destination address
// #5 screen : display amount
// #6 screen : display gas fee
// #7 screen : approve button
// #8 screen : reject button
UX_FLOW(ux_display_tx_aptos_account_transfer_flow,
        &ux_display_review_step,
        &ux_display_tx_type_step,
        &ux_display_function_step,
        &ux_display_receiver_step,
        &ux_display_amount_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

// FLOW to display coin_transfer transaction information:
// #1 screen : eye icon + "Review Transaction"
// #2 screen : display tx type
// #3 screen : display function name
// #4 screen : display coin type
// #5 screen : display destination address
// #6 screen : display amount
// #7 screen : display gas fee
// #8 screen : approve button
// #9 screen : reject button
UX_FLOW(ux_display_tx_coin_transfer_flow,
        &ux_display_review_step,
        &ux_display_tx_type_step,
        &ux_display_function_step,
        &ux_display_coin_type_step,
        &ux_display_receiver_step,
        &ux_display_amount_step,
        &ux_display_gas_fee_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

int ui_display_transaction() {
    g_validate_callback = &ui_action_validate_transaction;

    const int ret = ui_prepare_transaction();
    if (ret == UI_PREPARED) {
        ui_flow_verified_display(ux_display_blind_tx_default_flow);
        return 0;
    }

    return ret;
}

int ui_display_message() {
    if (N_storage.settings.show_full_message) {
        if (is_str_interrupted((const char *) G_context.tx_info.raw_tx,
                               G_context.tx_info.raw_tx_len)) {
            ui_flow_verified_display(ux_display_blind_message_flow);
        } else {
            ui_flow_display(ux_display_message_flow);
        }
    } else {
        memset(g_struct, 0, sizeof(g_struct));
        bool short_enough = G_context.tx_info.raw_tx_len < sizeof(g_struct);
        snprintf(g_struct,
                 sizeof(g_struct),
                 short_enough ? "%.*s" : "%.*s" DOTS,
                 short_enough ? G_context.tx_info.raw_tx_len : sizeof(g_struct) - sizeof(DOTS),
                 G_context.tx_info.raw_tx);
        PRINTF("Message: %s\n", g_struct);

        if (short_enough) {
            if (is_str_interrupted(g_struct, sizeof(g_struct))) {
                ui_flow_verified_display(ux_display_blind_short_message_flow);
            } else {
                ui_flow_display(ux_display_short_message_flow);
            }
        } else {
            ui_flow_verified_display(ux_display_blind_short_message_flow);
        }
    }

    return 0;
}

int ui_display_raw_message() {
    memset(g_struct, 0, sizeof(g_struct));
    const bool short_enough = sizeof(g_struct) >= 2 * G_context.tx_info.raw_tx_len + 1;
    if (short_enough) {
        format_hex(G_context.tx_info.raw_tx, G_context.tx_info.raw_tx_len,
                   g_struct, sizeof(g_struct));
    } else {
        const size_t cropped_bytes_len = (sizeof(g_struct) - sizeof(DOTS)) / 2;
        format_hex(G_context.tx_info.raw_tx, cropped_bytes_len,
                   g_struct, sizeof(g_struct));
        strcpy(g_struct + cropped_bytes_len * 2, DOTS);
    }
    PRINTF("Message: %s\n", g_struct);

    if (short_enough) {
        ui_flow_display(ux_display_raw_message_flow);
    } else {
        ui_flow_verified_display(ux_display_blind_raw_message_flow);
    }

    return 0;
}

int ui_display_entry_function() {
    const int ret = ui_prepare_entry_function();
    if (ret == UI_PREPARED) {
        ui_flow_verified_display(ux_display_blind_tx_entry_function_flow);
        return 0;
    }

    return ret;
}

int ui_display_tx_aptos_account_transfer() {
    const int ret = ui_prepare_tx_aptos_account_transfer();
    if (ret == UI_PREPARED) {
        ui_flow_display(ux_display_tx_aptos_account_transfer_flow);
        return 0;
    }

    return ret;
}

int ui_display_tx_coin_transfer() {
    const int ret = ui_prepare_tx_coin_transfer();
    if (ret == UI_PREPARED) {
        ui_flow_display(ux_display_tx_coin_transfer_flow);
        return 0;
    }

    return ret;
}

#endif
