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

#include "../globals.h"
#include "action/validate.h"
#include "constants.h"
#include "display.h"
#include "glyphs.h"
#include "menu.h"
#include "nbgl_display.h"
#include "nbgl_use_case.h"
#include "os.h"

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

        nbgl_useCaseReviewVerify(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                                 "Review transaction", NULL,
                                 "Sign transaction?", NULL, review_choice);
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

        nbgl_useCaseReviewVerify(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                                 "Review transaction", NULL,
                                 "Sign transaction?", NULL, review_choice);
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

        nbgl_useCaseReview(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                           "Review transaction to send Aptos", NULL,
                           "Sign transaction?", review_choice);
        return 0;
    }

    return ret;
}

void ui_listed_coin_transfer_flow_display() {
    pairs[0].item = "Transaction type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Function";
    pairs[1].value = g_function;
    pairs[2].item = "Amount";
    pairs[2].value = g_amount;
    pairs[3].item = "To";
    pairs[3].value = g_address;
    pairs[4].item = "Gas fee";
    pairs[4].value = g_gas_fee;

    pair_list.nbMaxLinesForValue = 0;
    pair_list.nbPairs = 5;
    pair_list.pairs = pairs;

    nbgl_useCaseReview(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                       "Review transaction to transfer coins", NULL,
                       "Sign transaction to transfer coins?", review_choice);
}

void ui_unlisted_coin_transfer_flow_display() {
    pairs[0].item = "Transaction type";
    pairs[0].value = g_tx_type;
    pairs[1].item = "Function";
    pairs[1].value = g_function;
    pairs[2].item = "Coin Type";
    pairs[2].value = g_struct;
    pairs[3].item = "Amount";
    pairs[3].value = g_amount;
    pairs[4].item = "To";
    pairs[4].value = g_address;
    pairs[5].item = "Gas fee";
    pairs[5].value = g_gas_fee;

    pair_list.nbMaxLinesForValue = 0;
    pair_list.nbPairs = 6;
    pair_list.pairs = pairs;

    nbgl_useCaseReview(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                       "Review transaction to transfer coins", NULL,
                       "Sign transaction to transfer coins?", review_choice);
}

int ui_display_tx_coin_transfer() {
    const int ret = ui_prepare_tx_coin_transfer();
    if (ret == UI_PREPARED) {
        if (g_is_token_listed) {
            ui_listed_coin_transfer_flow_display();
        } else {
            ui_unlisted_coin_transfer_flow_display();
        }
        return 0;
    }

    return ret;
}

int ui_display_tx_fungible_asset_transfer() {
    const int ret = ui_prepare_tx_fungible_asset_transfer();
    if (ret == UI_PREPARED) {
        if (g_is_token_listed) {
            ui_listed_coin_transfer_flow_display();
        } else {
            ui_unlisted_coin_transfer_flow_display();
        }
        return 0;
    }

    return ret;
}

static const char* get_delegation_title(
    entry_function_known_type_t function_type) {
    switch (function_type) {
        case FUNC_ADD_STAKE:
            return "Review transaction to delegate APT";
        case FUNC_UNLOCK_STAKE:
            return "Review transaction to undelegate APT";
        case FUNC_REACTIVATE_STAKE:
            return "Review transaction to reactivate APT";
        case FUNC_WITHDRAW_STAKE:
            return "Review transaction to withdraw APT";
        default:
            return "Review delegation pool transaction";
    }
}

static const char* get_delegation_sign_review(
    entry_function_known_type_t function_type) {
    switch (function_type) {
        case FUNC_ADD_STAKE:
            return "Sign transaction to delegate APT?";
        case FUNC_UNLOCK_STAKE:
            return "Sign transaction to undelegate APT?";
        case FUNC_REACTIVATE_STAKE:
            return "Sign transaction to reactivate APT?";
        case FUNC_WITHDRAW_STAKE:
            return "Sign transaction to withdraw APT?";
        default:
            return "Sign delegation pool transaction?";
    }
}

void ui_delegation_pool_flow_display(
    entry_function_known_type_t function_type) {
    PRINTF("ui_delegation_pool_flow_display");
    switch (function_type) {
        case FUNC_ADD_STAKE:
            pairs[0].item = "Delegate amount";
            break;
        case FUNC_UNLOCK_STAKE:
            pairs[0].item = "Undelegate amount";
            break;
        case FUNC_REACTIVATE_STAKE:
            pairs[0].item = "Reactivate stake";
            break;
        case FUNC_WITHDRAW_STAKE:
            pairs[0].item = "Withdraw amount";
            break;
        default:
            pairs[0].item = "Delegation pool";
            break;
    }
    pairs[0].value = g_amount;
    pairs[1].item = "Validator";
    pairs[1].value = g_address;
    pairs[2].item = "Gas fee";
    pairs[2].value = g_gas_fee;

    pair_list.nbMaxLinesForValue = 0;
    pair_list.nbPairs = 3;
    pair_list.pairs = pairs;

    nbgl_useCaseReview(TYPE_TRANSACTION, &pair_list, &ICON_APP_HOME,
                       get_delegation_title(function_type), NULL,
                       get_delegation_sign_review(function_type),
                       review_choice);
}

int ui_display_delegation_pool_transfer(
    entry_function_known_type_t function_type) {
    const int ret = ui_prepare_delegation_pool_transfer();
    if (ret == UI_PREPARED) {
        ui_delegation_pool_flow_display(function_type);
        return 0;
    }

    return ret;
}
#endif
