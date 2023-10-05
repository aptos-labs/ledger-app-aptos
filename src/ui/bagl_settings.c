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

#ifdef HAVE_BAGL

#include "os.h"

#include "settings.h"
#include "menu.h"
#include "../globals.h"

static const char* settings_submenu_getter(unsigned int idx);

static void settings_submenu_selector(unsigned int idx);

enum menu_options { MENU_SHOW_FULL_MSG = 0, MENU_ALLOW_BLIND_SIGNING = 1 };

static const char* const binary_choice_getter_values[] = {"No", "Yes", "Back"};

static const char* binary_choice_getter(unsigned int idx) {
    if (idx < ARRAYLEN(binary_choice_getter_values)) {
        return binary_choice_getter_values[idx];
    }
    return NULL;
}

static void show_full_message_selector(unsigned int idx) {
    if (idx == 0 || idx == 1) {
        settings_show_full_message_change((uint8_t) idx);
    }
    ux_menulist_init_select(0,
                            settings_submenu_getter,
                            settings_submenu_selector,
                            MENU_SHOW_FULL_MSG);
}

static void allow_blind_signing_selector(unsigned int idx) {
    if (idx == 0 || idx == 1) {
        settings_allow_blind_signing_change((uint8_t) idx);
    }
    ux_menulist_init_select(0,
                            settings_submenu_getter,
                            settings_submenu_selector,
                            MENU_ALLOW_BLIND_SIGNING);
}

static const char* const settings_submenu_getter_values[] = {
    "Show Full Message",
    "Allow Blind Signing",
    "Back",
};

static const char* settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

static void settings_submenu_selector(unsigned int idx) {
    switch (idx) {
        case MENU_SHOW_FULL_MSG:
            ux_menulist_init_select(0,
                                    binary_choice_getter,
                                    show_full_message_selector,
                                    N_storage.settings.show_full_message);
            break;
        case MENU_ALLOW_BLIND_SIGNING:
            ux_menulist_init_select(0,
                                    binary_choice_getter,
                                    allow_blind_signing_selector,
                                    N_storage.settings.allow_blind_signing);
            break;
        default:
            ui_menu_main();
    }
}

void ui_menu_settings() {
    ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector);
}

#endif
