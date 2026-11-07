//! Heltec WiFi LoRa 32 V4 (ESP32-S3 + SX1262) board support.
//!
//! * [`pins`] and [`fem`]: hardware knowledge, no dependencies, host-tested.
//! * `radio` (feature `lora`): SX1262 driver on `lora-phy`, generic over
//!   `embedded-hal` traits. The esp-hal code that creates the SPI device and
//!   pins lives in the firmware binary, not here.

#![no_std]
#![forbid(unsafe_code)]

pub mod fem;
pub mod pins;
#[cfg(feature = "lora")]
pub mod radio;
