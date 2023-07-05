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

#include "os.h"

#include "settings.h"
#include "../globals.h"

void settings_show_full_message_change(uint8_t value) {
    nvm_write((void*) &N_storage.settings.show_full_message, &value, sizeof(value));
}

void settings_allow_blind_signing_change(uint8_t value) {
    nvm_write((void*) &N_storage.settings.allow_blind_signing, &value, sizeof(value));
}
