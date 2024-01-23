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
#include "settings.h"
#include "menu.h"
#include "constants.h"
#include "../globals.h"

static use_case_review_ctx_t blind_sign_ctx;

nbgl_layoutTagValue_t pairs[6];
nbgl_layoutTagValueList_t pairList;
nbgl_pageInfoLongPress_t infoLongPress;

static void blind_sign_continue() {
    nbgl_useCaseReviewStart(blind_sign_ctx.icon,
                            blind_sign_ctx.review_title,
                            blind_sign_ctx.review_sub_title,
                            blind_sign_ctx.reject_text,
                            blind_sign_ctx.continue_callback,
                            blind_sign_ctx.reject_callback);
}

static void blind_sign_info() {
    nbgl_useCaseReviewStart(&C_round_warning_64px,
                            "Blind Signing",
                            "This operation cannot be\nsecurely interpreted by\nLedger Stax.\n"
                            "It might put your assets\nat risk.",
                            blind_sign_ctx.reject_text,
                            blind_sign_continue,
                            blind_sign_ctx.reject_callback);
}

static void blind_sign_choice(bool enable) {
    if (enable) {
        settings_allow_blind_signing_change(1);
        nbgl_useCaseStatus("BLIND SIGNING\nENABLED", true, blind_sign_info);
    } else {
        blind_sign_ctx.reject_callback();
    }
}

void nbgl_useCaseReviewVerify(const nbgl_icon_details_t *icon,
                              const char *review_title,
                              const char *review_sub_title,
                              const char *reject_text,
                              nbgl_callback_t continue_callback,
                              nbgl_callback_t reject_callback) {
    blind_sign_ctx.icon = icon;
    blind_sign_ctx.review_title = review_title;
    blind_sign_ctx.review_sub_title = review_sub_title;
    blind_sign_ctx.reject_text = reject_text;
    blind_sign_ctx.continue_callback = continue_callback;
    blind_sign_ctx.reject_callback = reject_callback;
    if (N_storage.settings.allow_blind_signing) {
        blind_sign_info();
    } else {
        nbgl_useCaseChoice(&C_round_warning_64px,
                           "Enable blind signing to\nauthorize this\noperation",
                           NULL,
                           "Enable blind signing",
                           reject_text,
                           blind_sign_choice);
    }
}

#endif
