use ledger_device_sdk::nvm::*;
use ledger_device_sdk::NVMData;

// NBGL uses one byte per switch: 0 is off, 1 is on.
const SETTINGS_SIZE: usize = 1;
#[link_section = ".nvm_data"]
static mut DATA: NVMData<AtomicStorage<[u8; SETTINGS_SIZE]>> =
    NVMData::new(AtomicStorage::new(&[0; SETTINGS_SIZE]));

#[derive(Clone, Copy)]
pub struct Settings;

impl Settings {
    /// Initializes empty NVM to off before NBGL reads or toggles the switch.
    #[inline(never)]
    pub fn get_mut(&mut self) -> &mut AtomicStorage<[u8; SETTINGS_SIZE]> {
        let data = &raw mut DATA;
        // SAFETY: the Ledger app is single-threaded; this static NVM slot is accessed
        // only through this getter and remains allocated for the app lifetime.
        let storage = unsafe { (*data).get_mut() };
        storage.get_or_init(&[0; SETTINGS_SIZE]);
        storage
    }
}
