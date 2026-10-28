/*****************************************************************************
 *   Ledger App Aptos.
 *   (c) 2023 Ledger SAS.
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
use crate::app_ui::sign::ui_display_tx;
use crate::swap::AptosCreateTxParams;
use crate::utils::Bip32Path;
use crate::AppSW;
use alloc::vec::Vec;
use ledger_device_sdk::ecc::Ed25519;
use ledger_device_sdk::io::{Command, CommandResponse};
use ledger_device_sdk::log;
use ledger_device_sdk::nbgl::NbglHomeAndSettings;

use serde::Deserialize;
use serde_json_core::from_slice;

// Three payload APDUs of at most 255 bytes each, after the path APDU.
const MAX_TRANSACTION_LEN: usize = 765;

#[derive(Deserialize)]
pub struct Tx<'a> {
    #[allow(dead_code)]
    nonce: u64,
    pub coin: &'a str,
    pub value: u64,
    #[serde(with = "hex::serde")]
    pub to: [u8; 32],
    pub memo: &'a str,
}

/// Transaction context holding state between APDU chunks.
pub struct TxContext<'a> {
    raw_tx: Vec<u8>,
    path: Bip32Path,
    review_finished: bool,
    pub home: NbglHomeAndSettings,
    /// Swap parameters if running in swap mode.
    /// Used to validate the transaction against the Exchange's request.
    pub swap_params: Option<&'a AptosCreateTxParams>,
}

impl<'a> TxContext<'a> {
    pub fn new() -> Self {
        Self {
            raw_tx: Vec::new(),
            path: Bip32Path::default(),
            review_finished: false,
            home: NbglHomeAndSettings::default(),
            swap_params: None,
        }
    }

    pub fn new_with_swap(params: &'a AptosCreateTxParams) -> Self {
        Self {
            swap_params: Some(params),
            ..Self::new()
        }
    }

    #[allow(dead_code)]
    pub fn finished(&self) -> bool {
        self.review_finished
    }

    fn reset(&mut self) {
        self.raw_tx.clear();
        self.path = Bip32Path::default();
        self.review_finished = false;
    }
}

/// Handler for the Sign Transaction APDU.
///
/// Receives transaction chunks, parses them, and signs the transaction.
///
/// # Swap Mode
///
/// If `ctx.swap_params` is present:
/// 1. Validates the parsed transaction against the swap parameters.
/// 2. If valid, bypasses the UI and signs immediately.
/// 3. If invalid, returns an error.
pub fn handler_sign_tx<'a>(
    command: Command<'a>,
    chunk: u8,
    more: bool,
    ctx: &mut TxContext,
) -> Result<CommandResponse<'a>, AppSW> {
    log::debug!("=> handler_sign_tx");
    let data = command.get_data();
    if chunk == 0 {
        log::debug!("Chunk 0: Path parsing");
        ctx.reset();
        ctx.path = data.try_into()?;
        Ok(command.into_response())
    } else {
        if ctx.raw_tx.len() + data.len() > MAX_TRANSACTION_LEN {
            return Err(AppSW::TxWrongLength);
        }

        ctx.raw_tx.extend(data);

        if more {
            ctx.review_finished = false;
            Ok(command.into_response())
        } else {
            log::debug!("Last chunk received, parsing tx");
            let (tx, _): (Tx, usize) = from_slice(&ctx.raw_tx).map_err(|_| AppSW::TxParsingFail)?;
            log::debug!("Tx parsed successfully");

            if let Some(params) = ctx.swap_params {
                ctx.review_finished = true;
                if let Err(error) = crate::swap::check_swap_params(params, &tx) {
                    let mut response = command.into_response();
                    error.append_to_response(&mut response)?;
                    Err(AppSW::SwapCheckingFail)
                } else {
                    log::debug!("Swap validation success, bypassing UI");
                    compute_signature_and_append(command.into_response(), ctx)
                }
            } else {
                log::debug!("Normal mode, showing UI");
                let comm = command.into_comm();

                if ui_display_tx(comm, &tx)? {
                    ctx.review_finished = true;
                    compute_signature_and_append(comm.begin_response(), ctx)
                } else {
                    ctx.review_finished = true;
                    Err(AppSW::Deny)
                }
            }
        }
    }
}

fn compute_signature_and_append<'a>(
    mut response: CommandResponse<'a>,
    ctx: &mut TxContext,
) -> Result<CommandResponse<'a>, AppSW> {
    log::debug!("Signing transaction");
    let key = Ed25519::derive_from_path_slip10(ctx.path.as_ref());
    let (signature, length) = key.sign(&ctx.raw_tx).map_err(|_| AppSW::SignatureFail)?;
    if length != 64 {
        return Err(AppSW::SignatureFail);
    }
    response.append(&[64])?.append(&signature)?;
    Ok(response)
}
