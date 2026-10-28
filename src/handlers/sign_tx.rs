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
use crate::app_ui::sign::ui_display_sign_payload;
use crate::swap::AptosCreateTxParams;
use crate::utils::Bip32Path;
use crate::{AppSW, MAX_TRANSACTION_PACKETS};
use ledger_device_sdk::ecc::Ed25519;
use ledger_device_sdk::io::{Command, CommandResponse};
use ledger_device_sdk::log::debug;
use ledger_device_sdk::nbgl::{NbglChoice, NbglHomeAndSettings, NbglStatus};

const MAX_TRANSACTION_LEN: usize = MAX_TRANSACTION_PACKETS as usize * 255;

const RAW_TRANSACTION_PREFIX: [u8; 32] = [
    181, 233, 125, 176, 127, 160, 189, 14, 85, 152, 170, 54, 67, 169, 188, 111, 102, 147, 189, 220,
    26, 159, 236, 158, 103, 74, 70, 30, 170, 0, 177, 147,
];
const RAW_TRANSACTION_WITH_DATA_PREFIX: [u8; 32] = [
    94, 250, 60, 79, 2, 248, 58, 15, 75, 45, 105, 252, 149, 198, 7, 204, 2, 130, 92, 196, 231, 190,
    83, 110, 240, 153, 45, 240, 80, 217, 230, 124,
];

/// Minimum byte length of a `RawTransaction` body after its 32-byte prefix.
///
/// The transport does not decode the body; this bound only rejects a body too
/// short to contain the fixed `RawTransaction` header fields.
const MIN_RAW_TRANSACTION_BODY_LEN: usize = 32 + 8 + 1 + 8 + 8 + 8 + 1;
/// `RawTransactionWithData` carries one variant byte before the transaction.
const MIN_RAW_TRANSACTION_WITH_DATA_BODY_LEN: usize = 1 + MIN_RAW_TRANSACTION_BODY_LEN;

// One APDU flow at a time is served by the SDK. Keeping the payload in BSS
// avoids asking the 8 KiB heap to hold v1's 18–27 KiB transactions.
static mut RAW_TRANSACTION: [u8; MAX_TRANSACTION_LEN] = [0; MAX_TRANSACTION_LEN];

/// Payload classification derived from the leading 32-byte signing prefix.
#[derive(Clone, Copy, Debug)]
pub enum SigningPayload {
    /// Any payload without a known Aptos transaction signing prefix.
    Message,
    /// A payload prefixed by SHA3-256("APTOS::RawTransaction").
    RawTransaction,
    /// A payload prefixed by SHA3-256("APTOS::RawTransactionWithData").
    RawTransactionWithData,
}

/// State for the chunked SIGN_TX flow.
pub struct TxContext<'a> {
    path: Bip32Path,
    len: usize,
    next_chunk: Option<u8>,
    review_finished: bool,
    payload: SigningPayload,
    pub home: NbglHomeAndSettings,
    pub swap_params: Option<&'a AptosCreateTxParams>,
}

impl<'a> TxContext<'a> {
    pub fn new() -> Self {
        Self {
            path: Bip32Path::default(),
            len: 0,
            next_chunk: None,
            review_finished: false,
            payload: SigningPayload::Message,
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

    pub fn finished(&self) -> bool {
        self.review_finished
    }

    /// Clear a pending SIGN_TX flow without leaving the current app screen.
    ///
    /// GET_PUBLIC_KEY clears v1's global signing state unconditionally. The
    /// home screen and Exchange parameters outlive a signing attempt.
    pub fn reset_signing(&mut self) {
        self.path = Bip32Path::default();
        self.len = 0;
        self.next_chunk = None;
        self.review_finished = false;
        self.payload = SigningPayload::Message;
    }

    /// Return whether the completed payload was classified as a message.
    pub fn is_message(&self) -> bool {
        matches!(self.payload, SigningPayload::Message)
    }

    fn payload(&self) -> &'static [u8] {
        payload_bytes(self.len)
    }

    fn append(&mut self, data: &[u8]) -> Result<(), AppSW> {
        let end = self
            .len
            .checked_add(data.len())
            .ok_or(AppSW::TxWrongLength)?;
        if end > MAX_TRANSACTION_LEN {
            return Err(AppSW::TxWrongLength);
        }
        // SAFETY: only the single APDU loop writes this buffer. The input is
        // copied before the command's borrowed APDU storage is reused.
        unsafe { RAW_TRANSACTION[self.len..end].copy_from_slice(data) };
        self.len = end;
        Ok(())
    }
}

fn payload_bytes(len: usize) -> &'static [u8] {
    // SAFETY: the SDK serializes APDUs and the single transaction flow owns
    // this static buffer. It is only read after all chunks have been copied.
    unsafe { &RAW_TRANSACTION[..len] }
}

/// Classify a payload by comparing its first 32 bytes with Aptos's signing
/// prefixes.
fn classify_payload(input: &[u8]) -> Result<SigningPayload, AppSW> {
    if let Some(body) = input.strip_prefix(&RAW_TRANSACTION_PREFIX) {
        if body.len() < MIN_RAW_TRANSACTION_BODY_LEN {
            return Err(AppSW::TxParsingFail);
        }
        Ok(SigningPayload::RawTransaction)
    } else if let Some(body) = input.strip_prefix(&RAW_TRANSACTION_WITH_DATA_PREFIX) {
        if body.len() < MIN_RAW_TRANSACTION_WITH_DATA_BODY_LEN {
            return Err(AppSW::TxParsingFail);
        }
        Ok(SigningPayload::RawTransactionWithData)
    } else {
        Ok(SigningPayload::Message)
    }
}

/// Handle one SIGN_TX APDU.
///
/// Chunk zero carries the derivation path. Later chunks carry payload bytes
/// and must be numbered consecutively. A rejected chunk does not consume its
/// sequence number or mutate the accumulated payload.
// Keep the handler out of the APDU loop frame; Nano X has only 2.8 KiB of
// stack once the v1-sized payload buffer is mapped.
#[inline(never)]
pub fn handler_sign_tx<'a>(
    command: Command<'a>,
    chunk: u8,
    more: bool,
    ctx: &mut TxContext,
) -> Result<CommandResponse<'a>, AppSW> {
    let data = command.get_data();
    debug!("SIGN_TX: chunk={} more={} len={}", chunk, more, data.len());

    if chunk == 0 {
        debug!("SIGN_TX: resetting context for path chunk");
        ctx.reset_signing();
        ctx.path = match data.try_into() {
            Ok(path) => path,
            Err(sw) => {
                debug!("SIGN_TX: path chunk rejected");
                return Err(sw);
            }
        };
        debug!("SIGN_TX: path accepted");
        ctx.next_chunk = Some(1);
        return Ok(command.into_response());
    }

    let Some(expected) = ctx.next_chunk else {
        debug!("SIGN_TX: payload chunk without active flow");
        return Err(AppSW::BadState);
    };
    if chunk != expected {
        debug!(
            "SIGN_TX: out-of-order chunk={} expected={}",
            chunk, expected
        );
        return Err(AppSW::WrongP1P2);
    }

    ctx.append(data)?;
    debug!("SIGN_TX: accepted chunk={} total_len={}", chunk, ctx.len);
    if more {
        ctx.next_chunk = expected.checked_add(1);
        return Ok(command.into_response());
    }
    ctx.next_chunk = None;
    debug!("SIGN_TX: final payload len={}", ctx.len);

    let payload = match classify_payload(ctx.payload()) {
        Ok(payload) => payload,
        Err(sw) => {
            debug!("SIGN_TX: payload classification failed");
            return Err(sw);
        }
    };
    ctx.payload = payload;
    debug!("SIGN_TX: payload classified as {:?}", payload);

    if let Some(params) = ctx.swap_params {
        debug!("SIGN_TX: validating Exchange transaction");
        let tx = crate::swap::parse_swap_transaction(ctx.payload())?;
        ctx.review_finished = true;
        crate::swap::check_swap_params(params, &tx).map_err(|_| AppSW::SwapCheckingFail)?;
        return compute_signature_and_append(command.into_response(), ctx);
    }

    let comm = command.into_comm();
    let mut settings = crate::settings::Settings;
    if !settings.blind_signing_enabled() {
        debug!("SIGN_TX: requesting Blind signing enablement");
        if !NbglChoice::new().show(
            comm,
            "Enable blind signing to authorize this operation?",
            "",
            "Enable blind signing",
            "Reject operation",
        ) {
            ctx.review_finished = true;
            return Err(AppSW::Deny);
        }
        settings.enable_blind_signing();
        debug!("SIGN_TX: Blind signing enabled");
        NbglStatus::new()
            .text("Blind signing enabled")
            .show(comm, true);
    }

    let approved = ui_display_sign_payload(comm, &payload)?;
    ctx.review_finished = true;
    if !approved {
        return Err(AppSW::Deny);
    }
    debug!("SIGN_TX: computing signature over {} bytes", ctx.len);
    compute_signature_and_append(comm.begin_response(), ctx)
}

fn compute_signature_and_append<'a>(
    mut response: CommandResponse<'a>,
    ctx: &TxContext,
) -> Result<CommandResponse<'a>, AppSW> {
    let key = Ed25519::derive_from_path_slip10(ctx.path.as_ref());
    let (signature, length) = key.sign(ctx.payload()).map_err(|_| AppSW::SignatureFail)?;
    if length != 64 {
        return Err(AppSW::SignatureFail);
    }
    response.append(&[64])?.append(&signature)?;
    Ok(response)
}
