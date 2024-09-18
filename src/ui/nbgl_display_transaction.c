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

static void review_choice(bool confirm) {
    if (confirm) {
        validate_transaction(true);
        nbgl_useCaseStatus("Transaction signed", true, ui_menu_main);
    } else {
        validate_transaction(false);
        nbgl_useCaseStatus("Transaction rejected", false, ui_menu_main);
    }
}

int ui_display_transaction() {
    const int ret = ui_prepare_transaction();
    if (ret == UI_PREPARED) {
        pairs[0].item = "Transaction type";
        pairs[0].value = g_tx_type;
        pairs[1].item = "Gas fee";
        pairs[1].value = g_gas_fee;

        pair_list.nbMaxLinesForValue = 0;
        pair_list.nbPairs = 2;
        pair_list.pairs = pairs;

        nbgl_useCaseReviewVerify(TYPE_TRANSACTION,
                                 &pair_list,
                                 &C_aptos_logo_64px,
                                 "Review transaction",
                                 NULL,
                                 "Sign transaction?",
                                 NULL,
                                 review_choice);
        return 0;
    }

    return ret;
}

int ui_display_entry_function() {
    const int ret = ui_prepare_entry_function();
    if (ret == UI_PREPARED) {
        pairs[0].item = "Transaction type";
        pairs[0].value = g_tx_type;
        pairs[1].item = "Function";
        pairs[1].value = g_function;
        pairs[2].item = "Gas fee";
        pairs[2].value = g_gas_fee;

        pair_list.nbMaxLinesForValue = 0;
        pair_list.nbPairs = 3;
        pair_list.pairs = pairs;

        nbgl_useCaseReviewVerify(TYPE_TRANSACTION,
                                 &pair_list,
                                 &C_aptos_logo_64px,
                                 "Review transaction",
                                 NULL,
                                 "Sign transaction?",
                                 NULL,
                                 review_choice);
        return 0;
    }

    return ret;
}

int ui_display_tx_aptos_account_transfer() {
    const int ret = ui_prepare_tx_aptos_account_transfer();
    if (ret == UI_PREPARED) {
        pairs[0].item = "Transaction type";
        pairs[0].value = g_tx_type;
        pairs[1].item = "Function";
        pairs[1].value = g_function;
        pairs[2].item = "Receiver";
        pairs[2].value = g_address;
        pairs[3].item = "Amount";
        pairs[3].value = g_amount;
        pairs[4].item = "Gas fee";
        pairs[4].value = g_gas_fee;

        pair_list.nbMaxLinesForValue = 0;
        pair_list.nbPairs = 5;
        pair_list.pairs = pairs;

        nbgl_useCaseReview(TYPE_TRANSACTION,
                           &pair_list,
                           &C_aptos_logo_64px,
                           "Review transaction to send Aptos",
                           NULL,
                           "Sign transaction?",
                           review_choice);
        return 0;
    }

    return ret;
}

int ui_display_tx_coin_transfer() {
    const int ret = ui_prepare_tx_coin_transfer();
    if (ret == UI_PREPARED) {
        pairs[0].item = "Transaction type";
        pairs[0].value = g_tx_type;
        pairs[1].item = "Function";
        pairs[1].value = g_function;
        pairs[2].item = "Coin type";
        pairs[2].value = g_struct;
        pairs[3].item = "Receiver";
        pairs[3].value = g_address;
        pairs[4].item = "Amount";
        pairs[4].value = g_amount;
        pairs[5].item = "Gas fee";
        pairs[5].value = g_gas_fee;

        pair_list.nbMaxLinesForValue = 0;
        pair_list.nbPairs = 6;
        pair_list.pairs = pairs;

        nbgl_useCaseReview(TYPE_TRANSACTION,
                           &pair_list,
                           &C_aptos_logo_64px,
                           "Review transaction to transfer coins",
                           NULL,
                           "Sign transaction?",
                           review_choice);
        return 0;
    }

    return ret;
}

#endif
