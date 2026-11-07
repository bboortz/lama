//! Heltec WiFi LoRa 32 V4 (ESP32-S3 + SX1262) board support.
//!
//! This crate currently holds the hardware-independent knowledge about the
//! board: the [`pins`] and the external front-end logic in [`fem`]. The
//! esp-hal / lora-phy glue that implements `lama_hal::Radio` comes next.

#![no_std]
#![forbid(unsafe_code)]

pub mod fem;
pub mod pins;
