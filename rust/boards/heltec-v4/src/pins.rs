//! GPIO numbers of the Heltec WiFi LoRa 32 V4 (ESP32-S3).
//!
//! Sources: Heltec product page and LNA wiki, Meshtastic `heltec_v4/variant.h`
//! and MeshCore. Verify against the schematic of your exact revision before
//! trusting a pin that is not used by the radio path.

// SX1262 on SPI
pub const LORA_SCK: u8 = 9;
pub const LORA_MISO: u8 = 11;
pub const LORA_MOSI: u8 = 10;
pub const LORA_NSS: u8 = 8;
pub const LORA_RST: u8 = 12;
pub const LORA_BUSY: u8 = 13;
pub const LORA_DIO1: u8 = 14;

/// TCXO supply voltage requested through DIO3, in millivolts (Meshtastic value).
pub const LORA_TCXO_MV: u16 = 1800;

// External front-end (PA/LNA), see `fem`
/// Front-end supply enable (on by default through a pull-up).
pub const FEM_POWER: u8 = 7;
/// Front-end chip enable; also read at start-up to detect the front-end type.
pub const FEM_CSD: u8 = 2;
/// TX/RX mode pin on boards with the GC1109 front-end (V4.2 and earlier).
pub const FEM_CTRL_GC1109: u8 = 46;
/// TX/RX mode pin on boards with the KCT8103L front-end (V4.3).
pub const FEM_CTRL_KCT8103L: u8 = 5;

// Other
pub const BUTTON: u8 = 0;
pub const BATTERY_ADC: u8 = 1;
pub const BATTERY_ADC_CTRL: u8 = 37;
/// Vext control (powers the OLED), active low, on the V4 with 2 MB PSRAM (R2).
pub const VEXT_CTRL_R2: u8 = 36;
/// Vext control on the V4-R8 (8 MB PSRAM).
pub const VEXT_CTRL_R8: u8 = 40;

// GNSS (V4 R2)
pub const GNSS_ENABLE: u8 = 34;
pub const GNSS_RESET: u8 = 42;
pub const GNSS_PPS: u8 = 41;
pub const GNSS_UART_TX: u8 = 38;
pub const GNSS_UART_RX: u8 = 39;
