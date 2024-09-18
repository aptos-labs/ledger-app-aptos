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

nbgl_contentTagValue_t pairs[6];
nbgl_contentTagValueList_t pair_list;

static void blind_sign_info() {
    nbgl_useCaseReviewBlindSigning(blind_sign_ctx.operation_type,
                                   blind_sign_ctx.tag_value_list,
                                   blind_sign_ctx.icon,
                                   blind_sign_ctx.review_title,
                                   blind_sign_ctx.review_sub_title,
                                   blind_sign_ctx.finish_title,
                                   blind_sign_ctx.tip_box,
                                   blind_sign_ctx.choice_callback);
}

static void blind_sign_choice(bool enable) {
    if (enable) {
        settings_allow_blind_signing_change(1);
        nbgl_useCaseStatus("Blind signing enabled", true, blind_sign_info);
    } else {
        blind_sign_ctx.choice_callback(false);
    }
}

void nbgl_useCaseReviewVerify(nbgl_operationType_t operation_type,
                              const nbgl_contentTagValueList_t *tag_value_list,
                              const nbgl_icon_details_t *icon,
                              const char *review_title,
                              const char *review_sub_title,
                              const char *finish_title,
                              const nbgl_tipBox_t *tip_box,
                              nbgl_choiceCallback_t choice_callback) {
    blind_sign_ctx.operation_type = operation_type;
    blind_sign_ctx.tag_value_list = tag_value_list;
    blind_sign_ctx.icon = icon;
    blind_sign_ctx.review_title = review_title;
    blind_sign_ctx.review_sub_title = review_sub_title;
    blind_sign_ctx.finish_title = finish_title;
    blind_sign_ctx.tip_box = tip_box;
    blind_sign_ctx.choice_callback = choice_callback;

    if (N_storage.settings.allow_blind_signing) {
        blind_sign_info();
    } else {
        nbgl_useCaseChoice(&C_Warning_64px,
                           "Enable blind signing to authorize this operation?",
                           NULL,
                           "Enable blind signing",
                           "Reject operation",
                           blind_sign_choice);
    }
}

#endif
