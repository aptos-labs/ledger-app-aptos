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

#include "../common/user_format.h"
#include "../globals.h"
#include "action/validate.h"
#include "constants.h"
#include "display.h"
#include "format.h"
#include "glyphs.h"
#include "menu.h"
#include "nbgl_display.h"
#include "nbgl_use_case.h"
#include "os.h"

#define DOTS "[...]"

static void review_choice(bool confirm) {
    if (confirm) {
        validate_transaction(true);
        nbgl_useCaseStatus("Message signed", true, ui_menu_main);
    } else {
        nbgl_useCaseStatus("Message rejected", false, ui_menu_main);
    }
}

int ui_display_message() {
    pairs[0].item = "Message";
    pairs[0].value = (const char*)G_context.tx_info.raw_tx;

    pair_list.nbMaxLinesForValue = 0;
    pair_list.nbPairs = 1;
    pair_list.pairs = pairs;

    if (is_str_interrupted((const char*)G_context.tx_info.raw_tx,
                           G_context.tx_info.raw_tx_len)) {
        nbgl_useCaseReviewVerify(TYPE_MESSAGE, &pair_list, &LARGE_REVIEW_ICON,
                                 "Review message", NULL, "Sign message?", NULL,
                                 review_choice);
    } else {
        nbgl_useCaseReview(TYPE_MESSAGE, &pair_list, &LARGE_REVIEW_ICON,
                           "Review message", NULL, "Sign message?",
                           review_choice);
    }

    return 0;
}

int ui_display_raw_message() {
    memset(g_struct, 0, sizeof(g_struct));
    const bool short_enough =
        sizeof(g_struct) >= 2 * G_context.tx_info.raw_tx_len + 1;
    if (short_enough) {
        format_hex(G_context.tx_info.raw_tx, G_context.tx_info.raw_tx_len,
                   g_struct, sizeof(g_struct));
    } else {
        const size_t cropped_bytes_len = (sizeof(g_struct) - sizeof(DOTS)) / 2;
        format_hex(G_context.tx_info.raw_tx, cropped_bytes_len, g_struct,
                   sizeof(g_struct));
        strncpy(g_struct + cropped_bytes_len * 2, DOTS, sizeof(DOTS));
    }

    pairs[0].item = "Raw message";
    pairs[0].value = g_struct;

    pair_list.nbMaxLinesForValue = 0;
    pair_list.nbPairs = 1;
    pair_list.pairs = pairs;

    if (!short_enough) {
        nbgl_useCaseReviewVerify(TYPE_MESSAGE, &pair_list, &LARGE_REVIEW_ICON,
                                 "Review message", NULL, "Sign message?", NULL,
                                 review_choice);
    } else {
        nbgl_useCaseReview(TYPE_MESSAGE, &pair_list, &LARGE_REVIEW_ICON,
                           "Review message", NULL, "Sign message?",
                           review_choice);
    }

    return 0;
}

#endif
