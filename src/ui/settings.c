#include "os.h"

#include "settings.h"
#include "menu.h"
#include "../globals.h"

static const char* const binary_choice_getter_values[] = {"No", "Yes", "Back"};

static const char* binary_choice_getter(unsigned int idx) {
    if (idx < ARRAYLEN(binary_choice_getter_values)) {
        return binary_choice_getter_values[idx];
    }
    return NULL;
}

static void show_full_message_change(uint8_t value) {
    nvm_write((void*) &N_storage.settings.show_full_message, &value, sizeof(value));
}

static void show_full_message_selector(unsigned int idx) {
    if (idx == 0 || idx == 1) {
        show_full_message_change((uint8_t) idx);
    }
    ux_menulist_init_select(0,
                            settings_submenu_getter,
                            settings_submenu_selector,
                            MENU_SHOW_FULL_MSG);
}

static void allow_blind_signing_change(uint8_t value) {
    nvm_write((void*) &N_storage.settings.allow_blind_signing, &value, sizeof(value));
}

static void allow_blind_signing_selector(unsigned int idx) {
    if (idx == 0 || idx == 1) {
        allow_blind_signing_change((uint8_t) idx);
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

const char* settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

void settings_submenu_selector(unsigned int idx) {
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
