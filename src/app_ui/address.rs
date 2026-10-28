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

use crate::AppSW;
use arrayvec::ArrayString;
use core::fmt::Write;

use ledger_device_sdk::include_gif;
use ledger_device_sdk::io::Comm;
use ledger_device_sdk::nbgl::{Field, NbglAddressReview, NbglGlyph};

pub fn ui_display_pk(comm: &mut Comm, addr: &[u8; 32], path: &[u32]) -> Result<bool, AppSW> {
    let mut addr_hex = [0u8; 66];
    addr_hex[..2].copy_from_slice(b"0x");
    hex::encode_to_slice(addr, &mut addr_hex[2..]).map_err(|_| AppSW::DisplayAddressFail)?;
    let address = core::str::from_utf8(&addr_hex).map_err(|_| AppSW::DisplayAddressFail)?;

    // Ten path components need at most 10 * (10 decimal digits + 1 apostrophe)
    // plus nine separators. Keep the full path on the stack for NBGL.
    let mut path_string = ArrayString::<128>::new();
    for (index, component) in path.iter().enumerate() {
        if index != 0 {
            path_string.push('/');
        }
        write!(&mut path_string, "{}", component & 0x7fff_ffff)
            .map_err(|_| AppSW::DisplayBip32PathFail)?;
        if component & 0x8000_0000 != 0 {
            path_string.push('\'');
        }
    }
    #[cfg(target_os = "apex_p")]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_48px.png", NBGL));
    #[cfg(any(target_os = "stax", target_os = "flex"))]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_64px.gif", NBGL));
    #[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/home_nano_nbgl.png", NBGL));

    let fields = [Field {
        name: "Derivation path",
        value: path_string.as_str(),
    }];
    Ok(NbglAddressReview::new()
        .glyph(&APP_ICON)
        .review_title("Verify Aptos address")
        .set_tag_value_list(&fields)
        .show(comm, address))
}
