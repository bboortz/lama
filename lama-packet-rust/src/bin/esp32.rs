#![no_std]
#![no_main]

#[cfg(feature = "esp32")]
use panic_halt as _;

#[unsafe(no_mangle)]
pub extern "C" fn main() -> ! {
    loop {}
}
