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
#include "../address.h"
#include "action/validate.h"

static void confirm_address(bool choice) {
    validate_pubkey(choice);
    nbgl_useCaseStatus(choice ? "Address verified" : "Address verification canceled",
                       choice,
                       ui_menu_main);
}

int ui_display_address() {
    const int ret = ui_prepare_address();
    if (ret == UI_PREPARED) {
        pairs[0].item = "Derivation path";
        pairs[0].value = g_bip32_path;

        pair_list.nbMaxLinesForValue = 0;
        pair_list.nbPairs = 1;
        pair_list.pairs = pairs;

        nbgl_useCaseAddressReview(g_address,
                                  &pair_list,
                                  &C_aptos_logo_64px,
                                  "Verify Aptos address",
                                  NULL,
                                  confirm_address);
        return 0;
    }

    return ret;
}

#endif
