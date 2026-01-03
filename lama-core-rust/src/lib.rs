#![cfg_attr(not(feature = "std"), no_std)]

// For tests on Linux/host
#[cfg(feature = "std")]
extern crate std;

// ESP32 panic handler (ONLY on xtensa, ONLY without std)
#[cfg(all(target_arch = "xtensa", not(feature = "std")))]
use panic_halt as _;

// Custom panic handler ONLY when:
// - no std
// - not using panic-halt (xtensa)
// - not in test mode (test uses std)
#[cfg(all(not(feature = "std"), not(target_arch = "xtensa"), not(test)))]
use core::panic::PanicInfo;

#[cfg(all(not(feature = "std"), not(target_arch = "xtensa"), not(test)))]
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

pub mod config;
pub use config::*;

#[unsafe(no_mangle)]
pub extern "C" fn lama_packet_callable() -> bool {
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lama_packet_callable() {
        let result = lama_packet_callable();
        assert_eq!(result, true);
    }
}
