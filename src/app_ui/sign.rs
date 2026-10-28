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
use alloc::borrow::Cow;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

use crate::bcs::ADDRESS_LEN;
use crate::handlers::sign_tx::SigningPayload;
use crate::transaction::{EntryFunction, Envelope, Payload, Transaction, OCTAS_PER_APT};
use crate::AppSW;

use core::str;

use ledger_device_sdk::include_gif;
use ledger_device_sdk::io::Comm;
use ledger_device_sdk::nbgl::{Field, NbglGlyph, NbglReview, NbglStreamingReview, TransactionType};

#[cfg(target_os = "apex_p")]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_48px.png", NBGL));
#[cfg(any(target_os = "stax", target_os = "flex"))]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_64px.gif", NBGL));
#[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/home_nano_nbgl.png", NBGL));

/// Longest ASCII message shown in a single NBGL field. Longer messages are
/// streamed so the Nano X heap never holds one oversized C string.
const MAX_DIRECT_MESSAGE_LEN: usize = 119;
/// Keep each streamed page small enough for the 2 KiB Nano X heap after the
/// SDK has copied the field name and value into C strings.
const MESSAGE_CHUNK_LEN: usize = 64;
/// v1 crops a long raw message to 57 bytes so 114 hexadecimal digits plus
/// `[...]` fit its 120-byte display buffer.
const BLIND_RAW_PREVIEW_BYTES: usize = 57;
const DOTS: &str = "[...]";

/// Display the text before the first NUL, matching v1's C-string display.
fn ascii_display_text(message: &[u8]) -> Result<&str, AppSW> {
    let end = message
        .iter()
        .position(|&byte| byte == 0)
        .unwrap_or(message.len());
    str::from_utf8(&message[..end]).map_err(|_| AppSW::TxParsingFail)
}

/// Build the field shown for a message payload.
fn message_field<'a>(
    payload: &SigningPayload,
    message: &'a [u8],
) -> Result<(&'static str, Cow<'a, str>), AppSW> {
    match payload {
        SigningPayload::ClearAsciiMessage | SigningPayload::BlindAsciiMessage => {
            Ok(("Message", Cow::Borrowed(ascii_display_text(message)?)))
        }
        SigningPayload::ShortRawMessage => {
            Ok(("Raw message", Cow::Owned(hex::encode_upper(message))))
        }
        SigningPayload::BlindRawMessage => {
            let mut value = hex::encode_upper(&message[..BLIND_RAW_PREVIEW_BYTES]);
            value.push_str(DOTS);
            Ok(("Raw message", Cow::Owned(value)))
        }
        _ => Err(AppSW::TxParsingFail),
    }
}

/// Return whether an ASCII message is long enough to require streaming.
fn message_requires_stream(payload: &SigningPayload, message: &[u8]) -> bool {
    matches!(
        payload,
        SigningPayload::ClearAsciiMessage | SigningPayload::BlindAsciiMessage
    ) && ascii_display_text(message).is_ok_and(|text| text.len() > MAX_DIRECT_MESSAGE_LEN)
}

/// Review a message that fits in a single NBGL review field.
#[inline(never)]
fn ui_display_message_direct(
    comm: &mut Comm,
    payload: &SigningPayload,
    message: &[u8],
) -> Result<bool, AppSW> {
    let (field_name, value) = message_field(payload, message)?;
    let fields = [Field {
        name: field_name,
        value: &value,
    }];
    let mut review = NbglReview::new()
        .titles("Review message", "", "Sign message?")
        .glyph(&APP_ICON)
        .tx_type(TransactionType::Message);
    if payload.requires_blind_signing() {
        review = review.blind();
    }
    Ok(review.show(comm, &fields))
}

/// Review a long ASCII message one chunk at a time.
#[inline(never)]
fn ui_display_message_stream(
    comm: &mut Comm,
    payload: &SigningPayload,
    message: &[u8],
) -> Result<bool, AppSW> {
    let (field_name, value) = message_field(payload, message)?;
    let mut review = NbglStreamingReview::new()
        .glyph(&APP_ICON)
        .tx_type(TransactionType::Message);
    if payload.requires_blind_signing() {
        review = review.blind();
    }
    if !review.start(comm, "Review message", None) {
        return Ok(false);
    }

    let bytes = value.as_bytes();
    let mut offset = 0;
    loop {
        let end = (offset + MESSAGE_CHUNK_LEN).min(bytes.len());
        let chunk = str::from_utf8(&bytes[offset..end]).map_err(|_| AppSW::TxParsingFail)?;
        let fields = [Field {
            name: field_name,
            value: chunk,
        }];
        // `next` would offer Nano users a skip page that jumps straight to the
        // final confirmation, letting them sign text they never read.
        // `continue_review` passes no skip callback.
        #[allow(deprecated)]
        if !review.continue_review(comm, &fields) {
            return Ok(false);
        }
        if end == bytes.len() {
            break;
        }
        offset = end;
    }
    Ok(review.finish(comm, "Sign message?"))
}

/// Title for the transaction type field, following v1's wording.
fn transaction_type(transaction: &Transaction) -> &'static str {
    match (transaction.envelope, &transaction.payload) {
        (Envelope::WithData, _) => "APTOS::RawTransactionWithData",
        (Envelope::Raw, Payload::EntryFunction(_)) => "Function call",
        (Envelope::Raw, Payload::Script) => "APTOS::RawTransaction [payload = SCRIPT]",
        (Envelope::Raw, Payload::Multisig) => "APTOS::RawTransaction [payload = MULTISIG]",
    }
}

/// Longest function name the review shows. v1 formats the name into a
/// 120-byte buffer, and Nano X's 2 KiB heap cannot hold a full-length
/// `0x<address>::<module>::<function>` twice (once here, once in the SDK's C
/// copy), so longer names are cut the way v1 cuts them.
const MAX_FUNCTION_NAME_LEN: usize = 119;

/// Format an entry function as `0x<address>::<module>::<function>`, with
/// leading zero bytes of the address dropped but at least one byte kept.
///
/// Kept out of line so its scratch buffers never sit in the review's frame:
/// Nano X's stack protector trips when that frame grows.
#[inline(never)]
fn function_name(function: &EntryFunction) -> String {
    let address = function.module_address;
    let first = address
        .iter()
        .position(|&byte| byte != 0)
        .unwrap_or(address.len() - 1);
    let mut hex = [0u8; 2 * ADDRESS_LEN];
    let hex = &mut hex[..2 * (ADDRESS_LEN - first)];
    // The output is exactly twice the input, so encoding cannot fail.
    let _ = hex::encode_to_slice(&address[first..], hex);
    hex.make_ascii_uppercase();

    let mut name = String::with_capacity(MAX_FUNCTION_NAME_LEN);
    // Every part is ASCII, so cutting at a byte never splits a character.
    for part in [
        "0x",
        str::from_utf8(hex).unwrap_or_default(),
        "::",
        function.module,
        "::",
        function.function,
    ] {
        let room = MAX_FUNCTION_NAME_LEN - name.len();
        name.push_str(&part[..part.len().min(room)]);
    }
    name
}

/// Format octas as APT with eight decimals.
fn gas_fee(octas: u64) -> String {
    format!("APT {}.{:08}", octas / OCTAS_PER_APT, octas % OCTAS_PER_APT)
}

/// Review a transaction: its type, the function it calls and the gas fee.
///
/// Everything shown comes from the strictly parsed bytes. Transactions are
/// blind-signed because they are not yet clear-signed.
#[inline(never)]
fn ui_display_transaction(comm: &mut Comm, bytes: &[u8]) -> Result<bool, AppSW> {
    let transaction = Transaction::parse(bytes).ok_or(AppSW::TxParsingFail)??;
    let function = match &transaction.payload {
        Payload::EntryFunction(function) => Some(function_name(function)),
        _ => None,
    };
    let fee = gas_fee(transaction.gas_fee);

    let mut fields = Vec::with_capacity(3);
    fields.push(Field {
        name: "Transaction type",
        value: transaction_type(&transaction),
    });
    if let Some(function) = &function {
        fields.push(Field {
            name: "Function",
            value: function,
        });
    }
    fields.push(Field {
        name: "Gas fee",
        value: &fee,
    });

    Ok(NbglReview::new()
        .titles("Review transaction", "", "Sign transaction?")
        .glyph(&APP_ICON)
        .tx_type(TransactionType::Transaction)
        .blind()
        .show(comm, &fields))
}

/// Review a signed payload.
#[inline(never)]
pub fn ui_display_sign_payload(
    comm: &mut Comm,
    payload: &SigningPayload,
    message: &[u8],
) -> Result<bool, AppSW> {
    if payload.is_message() {
        if message_requires_stream(payload, message) {
            ui_display_message_stream(comm, payload, message)
        } else {
            ui_display_message_direct(comm, payload, message)
        }
    } else {
        ui_display_transaction(comm, message)
    }
}
