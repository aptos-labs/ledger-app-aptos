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
use alloc::boxed::Box;
use alloc::vec::Vec;

use crate::bcs::ADDRESS_LEN;
use crate::handlers::sign_tx::SigningPayload;
use crate::tokens::{self, DEFAULT_DECIMALS};
use crate::transaction::{
    CoinType, EntryFunction, Envelope, Operation, Payload, Transaction, Transfer, APT_DECIMALS,
};
use crate::AppSW;

use arrayvec::ArrayVec;
use core::fmt::{self, Write};
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
        (Envelope::Raw, Payload::EntryFunction(function)) => match function.operation {
            None => "Function call",
            Some(Operation::AptTransfer(_)) => "APT transfer",
            Some(Operation::CoinTransfer { .. }) => "Coin transfer",
        },
        (Envelope::Raw, Payload::Script) => "APTOS::RawTransaction [payload = SCRIPT]",
        (Envelope::Raw, Payload::Multisig) => "APTOS::RawTransaction [payload = MULTISIG]",
    }
}

/// Longest qualified name the review shows. v1 formats function names and
/// coin types into 120-byte buffers, so longer names are cut the way v1
/// cuts them.
const MAX_QUALIFIED_NAME_LEN: usize = 119;

/// Room a transaction review's formatted values need: six rows, two of
/// which are cut qualified names and one a full address.
const REVIEW_TEXT_LEN: usize = 384;

/// Writes formatted text into a byte buffer, cutting it at a limit.
///
/// Running out of buffer is an error: a value cut by anything other than
/// its deliberate limit must never be shown as complete.
struct TextWriter<'a> {
    buffer: &'a mut [u8],
    len: usize,
    limit: usize,
}

impl fmt::Write for TextWriter<'_> {
    fn write_str(&mut self, text: &str) -> fmt::Result {
        // Every value is ASCII, so cutting at a byte never splits a character.
        let end = self.len + text.len().min(self.limit - self.len);
        if end > self.buffer.len() {
            return Err(fmt::Error);
        }
        self.buffer[self.len..end].copy_from_slice(&text.as_bytes()[..end - self.len]);
        self.len = end;
        Ok(())
    }
}

/// Hands out formatted values from a buffer the review borrows.
///
/// Nano X leaves the app a 2 KiB heap that the SDK's C copies of six rows
/// nearly fill, so the app's own copy of each value lives outside it.
struct Text<'a> {
    free: &'a mut [u8],
}

impl<'a> Text<'a> {
    /// Format `args`, cut at `limit` bytes.
    fn format(&mut self, limit: usize, args: fmt::Arguments) -> Result<&'a str, AppSW> {
        let mut writer = TextWriter {
            buffer: self.free,
            len: 0,
            limit,
        };
        writer
            .write_fmt(args)
            .map_err(|_| AppSW::DisplayAmountFail)?;
        let len = writer.len;
        let (text, free) = core::mem::take(&mut self.free).split_at_mut(len);
        self.free = free;
        str::from_utf8(text).map_err(|_| AppSW::DisplayAmountFail)
    }

    /// `0x<address>::<module>::<name>`, with leading zero bytes of the
    /// address dropped but at least one byte kept.
    fn qualified_name(
        &mut self,
        address: &[u8; ADDRESS_LEN],
        module: &str,
        name: &str,
    ) -> Result<&'a str, AppSW> {
        let first = address
            .iter()
            .position(|&byte| byte != 0)
            .unwrap_or(ADDRESS_LEN - 1);
        self.format(
            MAX_QUALIFIED_NAME_LEN,
            format_args!("0x{}::{module}::{name}", Hex(&address[first..])),
        )
    }

    fn function_name(&mut self, function: &EntryFunction) -> Result<&'a str, AppSW> {
        self.qualified_name(function.module_address, function.module, function.function)
    }

    /// An account address as `0x` and all 64 hexadecimal digits.
    fn address(&mut self, address: &[u8; ADDRESS_LEN]) -> Result<&'a str, AppSW> {
        self.format(usize::MAX, format_args!("0x{}", Hex(address)))
    }

    /// Octas as APT with eight decimals.
    fn apt_amount(&mut self, octas: u64) -> Result<&'a str, AppSW> {
        self.format(
            usize::MAX,
            format_args!("APT {}", Decimal(octas, APT_DECIMALS)),
        )
    }
}

/// Uppercase hexadecimal digits, as v1 formats addresses.
struct Hex<'a>(&'a [u8]);

impl fmt::Display for Hex<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for byte in self.0 {
            write!(f, "{byte:02X}")?;
        }
        Ok(())
    }
}

/// An integer amount as a decimal with exactly the given fraction digits,
/// like v1's `format_fpu64`.
struct Decimal(u64, u8);

impl fmt::Display for Decimal {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let Decimal(value, decimals) = *self;
        let scale = 10u64.checked_pow(u32::from(decimals)).ok_or(fmt::Error)?;
        write!(f, "{}", value / scale)?;
        if decimals > 0 {
            write!(
                f,
                ".{:0width$}",
                value % scale,
                width = usize::from(decimals)
            )?;
        }
        Ok(())
    }
}

/// The pages of a transaction review.
struct TransactionReview<'a> {
    title: &'static str,
    finish_title: &'static str,
    blind: bool,
    rows: ArrayVec<Field<'a>, 6>,
}

/// Prepare a transaction review from the strictly parsed bytes.
///
/// A known framework operation is clear-signed; anything else is reviewed by
/// its type, the function it calls and the gas fee, and must be blind-signed.
///
/// Boxed, and built in its own frame, so the review's frame stays small
/// while NBGL runs below it on Nano X's 2.8 KiB stack.
#[inline(never)]
fn prepare_transaction_review<'a>(
    bytes: &'a [u8],
    text: &'a mut [u8],
) -> Result<Box<TransactionReview<'a>>, AppSW> {
    let transaction = Transaction::parse(bytes).ok_or(AppSW::TxParsingFail)??;
    let mut text = Text { free: text };
    let fee = text.apt_amount(transaction.gas_fee)?;
    let mut rows = ArrayVec::new();
    rows.push(Field {
        name: "Transaction type",
        value: transaction_type(&transaction),
    });
    let (title, finish_title, blind) = match transaction.clear_signed_operation() {
        Some((function, operation)) => {
            rows.push(Field {
                name: "Function",
                value: text.function_name(function)?,
            });
            match operation {
                Operation::AptTransfer(transfer) => {
                    rows.push(Field {
                        name: "Receiver",
                        value: text.address(transfer.receiver)?,
                    });
                    rows.push(Field {
                        name: "Amount",
                        value: text.apt_amount(transfer.amount)?,
                    });
                    (
                        "Review transaction to send Aptos",
                        "Sign transaction?",
                        false,
                    )
                }
                Operation::CoinTransfer { coin, transfer } => {
                    push_coin_transfer_rows(&mut rows, &mut text, &coin, &transfer)?;
                    (
                        "Review transaction to transfer coins",
                        "Sign transaction to transfer coins?",
                        false,
                    )
                }
            }
        }
        None => {
            if let Payload::EntryFunction(function) = &transaction.payload {
                rows.push(Field {
                    name: "Function",
                    value: text.function_name(function)?,
                });
            }
            ("Review transaction", "Sign transaction?", true)
        }
    };
    rows.push(Field {
        name: "Gas fee",
        value: fee,
    });
    Ok(Box::new(TransactionReview {
        title,
        finish_title,
        blind,
        rows,
    }))
}

/// A listed coin shows its ticker and decimals; an unlisted coin shows its
/// full type and a raw amount.
fn push_coin_transfer_rows<'a>(
    rows: &mut ArrayVec<Field<'a>, 6>,
    text: &mut Text<'a>,
    coin: &CoinType,
    transfer: &Transfer,
) -> Result<(), AppSW> {
    let amount = match tokens::find_coin(coin) {
        Some(token) => text.format(
            usize::MAX,
            format_args!(
                "{} {}",
                token.ticker,
                Decimal(transfer.amount, token.decimals)
            ),
        )?,
        None => {
            rows.push(Field {
                name: "Coin Type",
                value: text.qualified_name(coin.address, coin.module, coin.name)?,
            });
            text.format(
                usize::MAX,
                format_args!("{}", Decimal(transfer.amount, DEFAULT_DECIMALS)),
            )?
        }
    };
    rows.push(Field {
        name: "Amount",
        value: amount,
    });
    rows.push(Field {
        name: "To",
        value: text.address(transfer.receiver)?,
    });
    Ok(())
}

/// Review a transaction, formatting its values into `scratch`.
///
/// The scratch is the tail of the payload buffer, which a transaction under
/// review leaves almost entirely free; a payload within [`REVIEW_TEXT_LEN`]
/// bytes of the buffer's end falls back to the heap.
#[inline(never)]
fn ui_display_transaction(
    comm: &mut Comm,
    bytes: &[u8],
    scratch: &mut [u8],
) -> Result<bool, AppSW> {
    let mut heap = Vec::new();
    let text = if scratch.len() >= REVIEW_TEXT_LEN {
        scratch
    } else {
        heap.resize(REVIEW_TEXT_LEN, 0);
        heap.as_mut_slice()
    };
    let review = prepare_transaction_review(bytes, text)?;
    let mut nbgl = NbglReview::new()
        .titles(review.title, "", review.finish_title)
        .glyph(&APP_ICON)
        .tx_type(TransactionType::Transaction);
    if review.blind {
        nbgl = nbgl.blind();
    }
    Ok(nbgl.show(comm, &review.rows))
}

/// Review a signed payload.
///
/// `scratch` is free memory a transaction review may format its values into.
#[inline(never)]
pub fn ui_display_sign_payload(
    comm: &mut Comm,
    payload: &SigningPayload,
    message: &[u8],
    scratch: &mut [u8],
) -> Result<bool, AppSW> {
    if payload.is_message() {
        if message_requires_stream(payload, message) {
            ui_display_message_stream(comm, payload, message)
        } else {
            ui_display_message_direct(comm, payload, message)
        }
    } else {
        ui_display_transaction(comm, message, scratch)
    }
}
