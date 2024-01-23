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

#ifdef HAVE_NBGL

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "glyphs.h"
#include "nbgl_use_case.h"

#include "nbgl_display.h"
#include "display.h"
#include "menu.h"
#include "constants.h"
#include "../globals.h"
#include "action/validate.h"

static void confirm_transaction_rejection(void) {
    validate_transaction(false);
    nbgl_useCaseStatus("Transaction rejected", false, ui_menu_main);
}

static void ask_transaction_rejection_confirmation(void) {
    nbgl_useCaseConfirm("Reject transaction?",
                        NULL,
                        "Yes, Reject",
                        "Go back to transaction",
                        confirm_transaction_rejection);
}

static void review_choice(bool confirm) {
    if (confirm) {
        validate_transaction(true);
        nbgl_useCaseStatus("TRANSACTION\nSIGNED", true, ui_menu_main);
    } else {
        ask_transaction_rejection_confirmation();
    }
}

static void review_default_continue(void) {
    pairs[0].item = "Transaction Type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Gas Fee";
    pairs[1].value = g_gas_fee;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 2;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_aptos_logo_64px;
    infoLongPress.text = "Sign transaction";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", review_choice);
}

static void review_entry_function_continue(void) {
    pairs[0].item = "Transaction Type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Function";
    pairs[1].value = g_function;
    pairs[2].item = "Gas Fee";
    pairs[2].value = g_gas_fee;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 3;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_aptos_logo_64px;
    infoLongPress.text = "Sign transaction";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", review_choice);
}

static void review_tx_aptos_account_transfer_continue(void) {
    pairs[0].item = "Transaction Type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Function";
    pairs[1].value = g_function;
    pairs[2].item = "Receiver";
    pairs[2].value = g_address;
    pairs[3].item = "Amount";
    pairs[3].value = g_amount;
    pairs[4].item = "Gas Fee";
    pairs[4].value = g_gas_fee;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 5;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_aptos_logo_64px;
    infoLongPress.text = "Sign transaction";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", review_choice);
}

static void review_tx_coin_transfer_continue(void) {
    pairs[0].item = "Transaction Type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Function";
    pairs[1].value = g_function;
    pairs[2].item = "Coin Type";
    pairs[2].value = g_struct;
    pairs[3].item = "Receiver";
    pairs[3].value = g_address;
    pairs[4].item = "Amount";
    pairs[4].value = g_amount;
    pairs[5].item = "Gas Fee";
    pairs[5].value = g_gas_fee;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 6;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_aptos_logo_64px;
    infoLongPress.text = "Sign transaction";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", review_choice);
}

int ui_display_transaction() {
    const int ret = ui_prepare_transaction();
    if (ret == UI_PREPARED) {
        nbgl_useCaseReviewVerify(&C_aptos_logo_64px,
                                 "Review transaction",
                                 NULL,
                                 "Reject transaction",
                                 review_default_continue,
                                 ask_transaction_rejection_confirmation);
        return 0;
    }

    return ret;
}

int ui_display_entry_function() {
    const int ret = ui_prepare_entry_function();
    if (ret == UI_PREPARED) {
        nbgl_useCaseReviewVerify(&C_aptos_logo_64px,
                                 "Review transaction",
                                 NULL,
                                 "Reject transaction",
                                 review_entry_function_continue,
                                 ask_transaction_rejection_confirmation);
        return 0;
    }

    return ret;
}

int ui_display_tx_aptos_account_transfer() {
    const int ret = ui_prepare_tx_aptos_account_transfer();
    if (ret == UI_PREPARED) {
        nbgl_useCaseReviewStart(&C_aptos_logo_64px,
                                "Review transaction\nto send Aptos",
                                NULL,
                                "Reject transaction",
                                review_tx_aptos_account_transfer_continue,
                                ask_transaction_rejection_confirmation);
        return 0;
    }

    return ret;
}

int ui_display_tx_coin_transfer() {
    const int ret = ui_prepare_tx_coin_transfer();
    if (ret == UI_PREPARED) {
        nbgl_useCaseReviewStart(&C_aptos_logo_64px,
                                "Review transaction\nto transfer coins",
                                NULL,
                                "Reject transaction",
                                review_tx_coin_transfer_continue,
                                ask_transaction_rejection_confirmation);
        return 0;
    }

    return ret;
}

#endif
