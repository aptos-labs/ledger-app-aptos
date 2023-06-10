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

#include "extend.h"
#include "../settings.h"

void ui_action_allow_blind_signing(const ux_flow_step_t *const *steps) {
    settings_allow_blind_signing_change(1);

    // Passed UX_FLOW steps are expected to contain a blind signing warning on the first step.
    // Skip it for better UX here.
    ux_flow_init(0, steps, steps[1]);
}