
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

#include "os.h"
#include "io.h"
#include "glyphs.h"
#include "nbgl_use_case.h"

#include "../globals.h"
#include "menu.h"
#include "settings.h"

#define SETTINGS_PAGE_NUMBER 2

enum {
    SWITCH_BLIND_SIGNING = 0,
    SWITCHES_COUNT,
};

enum {
    TOKEN_BLIND_SIGNING = FIRST_USER_TOKEN,
};

static const char* const INFO_TYPES[] = {"Version", "Developer", "Copyright"};
static const char* const INFO_CONTENTS[] = {APPVERSION,
                                            "Pontem Network",
                                            "(c) 2022 Pontem Network"};

static nbgl_layoutSwitch_t g_switches[SWITCHES_COUNT];

static bool settings_nav_callback(uint8_t page, nbgl_pageContent_t* content) {
    switch (page) {
        case 0:
            content->type = INFOS_LIST;
            content->infosList.nbInfos = ARRAYLEN(INFO_TYPES);
            content->infosList.infoTypes = (const char**) INFO_TYPES;
            content->infosList.infoContents = (const char**) INFO_CONTENTS;

            return true;
        case 1:
            if (N_storage.settings.allow_blind_signing == 0) {
                g_switches[SWITCH_BLIND_SIGNING].initState = OFF_STATE;
            } else {
                g_switches[SWITCH_BLIND_SIGNING].initState = ON_STATE;
            }
            content->type = SWITCHES_LIST;
            content->switchesList.nbSwitches = SWITCHES_COUNT;
            content->switchesList.switches = g_switches;

            return true;
        default:
            break;
    }

    return false;
}

static void settings_controls_callback(int token, uint8_t index) {
    switch (token) {
        case TOKEN_BLIND_SIGNING:
            if (index == 0 || index == 1) {
                settings_allow_blind_signing_change(index);
            }
            break;
        default:
            break;
    }
}

void app_quit(void) {
    os_sched_exit(-1);
}

void ui_menu_main(void) {
    nbgl_useCaseHome(APPNAME, &C_aptos_logo_64px, NULL, true, ui_menu_settings, app_quit);
}

void ui_menu_settings(void) {
    g_switches[SWITCH_BLIND_SIGNING].text = "Blind signing";
    g_switches[SWITCH_BLIND_SIGNING].subText = "Enable blind signing";
    g_switches[SWITCH_BLIND_SIGNING].token = TOKEN_BLIND_SIGNING;
    g_switches[SWITCH_BLIND_SIGNING].tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseSettings(APPNAME " settings",
                         0,
                         SETTINGS_PAGE_NUMBER,
                         false,
                         ui_menu_main,
                         settings_nav_callback,
                         settings_controls_callback);
}

#endif
