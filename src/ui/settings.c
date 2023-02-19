#include "os.h"

#include "settings.h"
#include "menu.h"
#include "../globals.h"

static const char* const show_full_message_getter_values[] = {"No", "Yes", "Back"};

static const char* show_full_message_getter(unsigned int idx) {
    if (idx < ARRAYLEN(show_full_message_getter_values)) {
        return show_full_message_getter_values[idx];
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

static const char* const settings_submenu_getter_values[] = {
    "Show Full Message",
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
                                    show_full_message_getter,
                                    show_full_message_selector,
                                    N_storage.settings.show_full_message);
            break;
        default:
            ui_menu_main();
    }
}
