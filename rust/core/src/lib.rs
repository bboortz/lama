//! Hardware-independent core of LAMA.
//!
//! * [`packet`]: explicit wire format (encode/decode, no padding, little-endian)
//! * [`stats`]: per-node link quality (RSSI, SNR, loss) in fixed-point, no heap
//! * [`state`]: the TX/RX state machine as pure event -> action logic
//!
//! This crate is `no_std`, has no dependencies and never touches hardware, so
//! it runs unchanged in host tests and on every board.

#![no_std]
#![forbid(unsafe_code)]

pub mod packet;
pub mod state;
pub mod stats;
