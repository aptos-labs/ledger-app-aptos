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
use crate::handlers::sign_tx::SigningPayload;
use crate::AppSW;

use ledger_device_sdk::include_gif;
use ledger_device_sdk::io::Comm;
use ledger_device_sdk::nbgl::{Field, NbglGlyph, NbglReview, TransactionType};

#[cfg(target_os = "apex_p")]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_48px.png", NBGL));
#[cfg(any(target_os = "stax", target_os = "flex"))]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_64px.gif", NBGL));
#[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
const APP_ICON: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/home_nano_nbgl.png", NBGL));

/// Review a blind-signing payload.
///
/// The transport signs the exact submitted bytes and does not decode message
/// or transaction content.
#[inline(never)]
pub fn ui_display_sign_payload(comm: &mut Comm, payload: &SigningPayload) -> Result<bool, AppSW> {
    let (title, finish, field_name, field_value, tx_type) = match payload {
        SigningPayload::Message => (
            "Review message",
            "Sign message?",
            "Message",
            "Message",
            TransactionType::Message,
        ),
        SigningPayload::RawTransaction => (
            "Review transaction",
            "Sign transaction?",
            "Transaction type",
            "RawTransaction",
            TransactionType::Transaction,
        ),
        SigningPayload::RawTransactionWithData => (
            "Review transaction",
            "Sign transaction?",
            "Transaction type",
            "RawTransactionWithData",
            TransactionType::Transaction,
        ),
    };
    let fields = [Field {
        name: field_name,
        value: field_value,
    }];

    Ok(NbglReview::new()
        .titles(title, "", finish)
        .glyph(&APP_ICON)
        .tx_type(tx_type)
        .blind()
        .show(comm, &fields))
}
