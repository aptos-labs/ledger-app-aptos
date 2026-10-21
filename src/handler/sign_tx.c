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

#include "sign_tx.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <string.h>   // memset, explicit_bzero

#include "../address.h"
#include "../crypto.h"
#include "../globals.h"
#include "../sw.h"
#include "../swap/handle_swap_sign_transaction.h"
#include "../transaction/deserialize.h"
#include "../transaction/types.h"
#include "../ui/action/validate.h"
#include "../ui/display.h"
#include "buffer.h"
#include "cx.h"
#include "os.h"

#ifdef HAVE_SWAP
#include "handle_swap_sign_transaction.h"
#include "swap.h"
#endif

int handler_sign_tx(buffer_t* cdata, uint8_t chunk, bool more) {
    PRINTF("handler_sign_tx called\n");
#ifdef HAVE_SWAP
    if (G_called_from_swap) {
        PRINTF("handler_sign_tx called from Exchange App \n");
    }
#endif

    static uint8_t prev_chunk = 0;  // no need to burden the global context

    if (chunk == 0) {  // first APDU, parse BIP32 path
        explicit_bzero(&G_context, sizeof(G_context));
        G_context.req_type = CONFIRM_TRANSACTION;
        G_context.state = STATE_NONE;
        prev_chunk = chunk;

        if (!buffer_read_u8(cdata, &G_context.bip32_path_len) ||
            !buffer_read_bip32_path(cdata, G_context.bip32_path,
                                    (size_t)G_context.bip32_path_len)) {
            // unable to recover from this error, reset the context
            G_context.req_type = REQUEST_UNDEFINED;
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        if (!validate_aptos_bip32_path(G_context.bip32_path,
                                       G_context.bip32_path_len)) {
            G_context.req_type = REQUEST_UNDEFINED;
            return io_send_sw(SW_GET_PUB_KEY_FAIL);
        }

        return io_send_sw(SW_OK);
    } else {  // parse transaction
        if (G_context.req_type != CONFIRM_TRANSACTION) {
            // there may be data in the global context's union, reset the
            // context anyway
            G_context.req_type = REQUEST_UNDEFINED;
            return io_send_sw(SW_BAD_STATE);
        }
        if (G_context.state == STATE_PARSED ||
            G_context.state == STATE_APPROVED) {
            // should not get here, double check, context should already be
            // reset
            return io_send_sw(SW_BAD_STATE);
        }
        if (chunk != prev_chunk + 1) {
            // give a chance to resend a chunk with the correct sequence number
            return io_send_sw(SW_WRONG_P1P2);
        }
        prev_chunk = chunk;

        if (G_context.tx_info.raw_tx_len + cdata->size >
                sizeof(G_context.tx_info.raw_tx) ||
            !buffer_move(
                cdata, G_context.tx_info.raw_tx + G_context.tx_info.raw_tx_len,
                cdata->size)) {
            // copying did not happen, allow the smaller chunk to be resent
            return io_send_sw(SW_WRONG_TX_LENGTH);
        }
        G_context.tx_info.raw_tx_len += cdata->size;

        if (more) {
            // more APDUs with transaction part are expected.
            // Send a SW_OK to signal that we have received the chunk
            return io_send_sw(SW_OK);

        } else {
            // last APDU for this transaction, let's parse, display and request
            // a sign confirmation

            buffer_t buf = {.ptr = G_context.tx_info.raw_tx,
                            .size = G_context.tx_info.raw_tx_len,
                            .offset = 0};

            parser_status_e status =
                transaction_deserialize(&buf, &G_context.tx_info.transaction);
            PRINTF("Parsing status: %d.\n", status);
            if (status != PARSING_OK) {
                // reset the context to prevent sending the "last" chunk
                // multiple times
                G_context.req_type = REQUEST_UNDEFINED;
                return io_send_sw(SW_TX_PARSING_FAIL);
            }

            G_context.state = STATE_PARSED;

#ifdef HAVE_SWAP
            // If we are in swap context, do not redisplay the message data
            // Instead, ensure they are identical with what was previously
            // displayed
            if (G_called_from_swap) {
                if (G_swap_response_ready) {
                    // Safety against trying to make the app sign multiple TX
                    // This code should never be triggered as the app is
                    // supposed to exit after sending the signed transaction
                    PRINTF("Safety against double signing triggered\n");
                    io_send_sw(SW_SWAP_CHECKING_FAIL);
                    os_sched_exit(-1);
                } else {
                    // We will quit the app after this transaction, whether it
                    // succeeds or fails
                    PRINTF(
                        "Swap response is ready, the app will quit after the "
                        "next send\n");
                    // This boolean will make the io_send_sw family instant
                    // reply + return to exchange
                    G_swap_response_ready = true;
                }
                if (swap_check_validity()) {
                    PRINTF("Swap response validated\n");
                    validate_transaction(true);
                } else {
                    PRINTF("swap_check_validity failed\n");
                    // Failsafe
                    io_send_sw(SW_SWAP_CHECKING_FAIL);
                    swap_finalize_exchange_sign_transaction(false);
                }

                return 0;
            } else {
                return ui_display_transaction();
            }
#else
            int ui_status = ui_display_transaction();
            G_context.req_type =
                REQUEST_UNDEFINED;  // all the work is done, reset the context
            return ui_status;
#endif
        }
    }

    return 0;
}
