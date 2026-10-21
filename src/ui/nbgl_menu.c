
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

#include "../globals.h"
#include "display.h"
#include "glyphs.h"
#include "io.h"
#include "menu.h"
#include "nbgl_use_case.h"
#include "os.h"
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
static const char* const INFO_CONTENTS[] = {APPVERSION, "Ledger",
                                            "(c) 2024 Ledger"};

static nbgl_contentSwitch_t g_switches[SWITCHES_COUNT];

static const nbgl_contentInfoList_t g_infos_list = {
    .infoTypes = INFO_TYPES,
    .infoContents = INFO_CONTENTS,
    .nbInfos = ARRAYLEN(INFO_TYPES),
};

static void settings_controls_callback(int token, uint8_t index, int page);

static const nbgl_content_t g_setting_contents_list[] = {{
    .type = SWITCHES_LIST,
    .content =
        {
            .switchesList =
                {
                    .switches = g_switches,
                    .nbSwitches = ARRAYLEN(g_switches),
                },
        },
    .contentActionCallback = settings_controls_callback,
}};

static const nbgl_genericContents_t g_setting_contents = {
    .callbackCallNeeded = false,
    .contentsList = g_setting_contents_list,
    .nbContents = 1,
};

static void settings_controls_callback(int token, uint8_t index,
                                       __attribute__((unused)) int page) {
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

void app_quit(void) { os_sched_exit(-1); }

void ui_menu_main(void) {
    g_switches[SWITCH_BLIND_SIGNING].text = "Blind signing";
    g_switches[SWITCH_BLIND_SIGNING].subText = "Enable blind signing";
    g_switches[SWITCH_BLIND_SIGNING].token = TOKEN_BLIND_SIGNING;
    g_switches[SWITCH_BLIND_SIGNING].initState =
        N_storage.settings.allow_blind_signing == 0 ? OFF_STATE : ON_STATE;

    nbgl_useCaseHomeAndSettings(APPNAME, &ICON_APP_HOME, NULL, INIT_HOME_PAGE,
                                &g_setting_contents, &g_infos_list, NULL,
                                app_quit);
}

#endif
