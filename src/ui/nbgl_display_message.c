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
#include "format.h"
#include "glyphs.h"
#include "nbgl_use_case.h"

#include "nbgl_display.h"
#include "display.h"
#include "menu.h"
#include "constants.h"
#include "../globals.h"
#include "action/validate.h"
#include "../common/user_format.h"

#define DOTS "[...]"

static void confirm_message_rejection(void) {
    validate_transaction(false);
    nbgl_useCaseStatus("Message rejected", false, ui_menu_main);
}

static void ask_message_rejection_confirmation(void) {
    nbgl_useCaseConfirm("Reject message?",
                        NULL,
                        "Yes, Reject",
                        "Go back to message",
                        confirm_message_rejection);
}

static void review_choice(bool confirm) {
    if (confirm) {
        validate_transaction(true);
        nbgl_useCaseStatus("MESSAGE\nSIGNED", true, ui_menu_main);
    } else {
        ask_message_rejection_confirmation();
    }
}

static void review_message_continue(void) {
    pairs[0].item = "Message";
    pairs[0].value = (const char *) G_context.tx_info.raw_tx;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 1;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_Message_64px;
    infoLongPress.text = "Sign message";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject message", review_choice);
}

static void review_raw_message_continue(void) {
    pairs[0].item = "Raw message";
    pairs[0].value = g_struct;

    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = 1;
    pairList.pairs = pairs;

    infoLongPress.icon = &C_Message_64px;
    infoLongPress.text = "Sign message";
    infoLongPress.longPressText = "Hold to sign";

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject message", review_choice);
}

int ui_display_message() {
    if (is_str_interrupted((const char *) G_context.tx_info.raw_tx, G_context.tx_info.raw_tx_len)) {
        nbgl_useCaseReviewVerify(&C_Message_64px,
                                 "Review message",
                                 NULL,
                                 "Reject message",
                                 review_message_continue,
                                 ask_message_rejection_confirmation);
    } else {
        nbgl_useCaseReviewStart(&C_Message_64px,
                                "Review message",
                                NULL,
                                "Reject message",
                                review_message_continue,
                                ask_message_rejection_confirmation);
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

    if (!short_enough) {
        nbgl_useCaseReviewVerify(&C_Message_64px,
                                 "Review message",
                                 NULL,
                                 "Reject message",
                                 review_raw_message_continue,
                                 ask_message_rejection_confirmation);
    } else {
        nbgl_useCaseReviewStart(&C_Message_64px,
                                "Review message",
                                NULL,
                                "Reject message",
                                review_raw_message_continue,
                                ask_message_rejection_confirmation);
    }

    return 0;
}

#endif
