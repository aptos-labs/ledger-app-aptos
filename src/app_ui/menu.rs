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

use ledger_device_sdk::include_gif;
use ledger_device_sdk::io::Comm;

use crate::settings::Settings;
use ledger_device_sdk::nbgl::{NbglGlyph, NbglHomeAndSettings};

#[inline(never)]
pub fn ui_menu_main(_: &mut Comm) -> NbglHomeAndSettings {
    #[cfg(target_os = "apex_p")]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_48px.png", NBGL));
    #[cfg(any(target_os = "stax", target_os = "flex"))]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/aptos_logo_64px.gif", NBGL));
    #[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
    const APP_ICON: NbglGlyph =
        NbglGlyph::from_include(include_gif!("glyphs/home_nano_nbgl.png", NBGL));

    let settings_strings = [["Blind signing", "Enable blind signing"]];
    let mut settings = Settings;

    NbglHomeAndSettings::new()
        .glyph(&APP_ICON)
        .settings(settings.get_mut(), &settings_strings)
        .app_name("Aptos")
        .info_list(&[
            ("Version", env!("CARGO_PKG_VERSION")),
            ("Developer", "Ledger"),
            ("Copyright", "(c) 2024 Ledger"),
        ])
}
