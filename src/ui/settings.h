#pragma once

enum menu_options { MENU_SHOW_FULL_MSG = 0, MENU_ALLOW_BLIND_SIGNING = 1 };

const char* settings_submenu_getter(unsigned int idx);

void settings_submenu_selector(unsigned int idx);

void settings_show_full_message_change(uint8_t value);

void settings_allow_blind_signing_change(uint8_t value);
