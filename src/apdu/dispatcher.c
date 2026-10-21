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

#include "dispatcher.h"

#include <stdbool.h>
#include <stdint.h>

#include "../constants.h"
#include "../globals.h"
#include "../handler/get_app_name.h"
#include "../handler/get_public_key.h"
#include "../handler/get_version.h"
#include "../handler/sign_tx.h"
#include "../sw.h"
#include "../types.h"
#include "buffer.h"
#include "io.h"

int apdu_dispatcher(const command_t* cmd) {
    PRINTF("Inside Aptos apdu_dispatcher\n");
    if (cmd->cla != CLA) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
    }

    buffer_t buf = {0};
    PRINTF("Dispatching APDU for specific function call\n");
    switch (cmd->ins) {
        case GET_VERSION:
            PRINTF("GET_VERSION\n");
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            return handler_get_version();
        case GET_APP_NAME:
            PRINTF("GET_APP_NAME\n");
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            return handler_get_app_name();
        case GET_PUBLIC_KEY:
            PRINTF("GET_PUBLIC_KEY\n");
            if (cmd->p1 > 1 || cmd->p2 > 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handler_get_public_key(&buf, (bool)cmd->p1);
        case SIGN_TX:
            PRINTF("SIGN_TX\n");
            if ((cmd->p1 == P1_START && cmd->p2 != P2_MORE) ||  //
                cmd->p1 >= P1_MAX ||                            //
                (cmd->p2 != P2_LAST && cmd->p2 != P2_MORE)) {
                PRINTF("Wrong P1 & P2 combo\n");
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                PRINTF("Lacking data\n");
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;
            PRINTF(
                "Inside Aptos apdu_dispatcher: ready to call "
                "handler_sign_tx\n");
            return handler_sign_tx(&buf, cmd->p1, (bool)(cmd->p2 & P2_MORE));
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
