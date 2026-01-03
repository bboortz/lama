// src/config.rs

// NVS functions removed - use Arduino Preferences API instead
// For ESP32 Arduino builds, config is loaded/saved from Arduino C++ code
// Rust provides the Config struct and getters/setters only

#[repr(C)]
pub struct Config {
    pub node_id: u8,
    pub network_id: u8,
    pub capabilities: u8,
    pub user: [u8; 16],

    pub oled_pin_sda: u8,
    pub oled_pin_scl: u8,
    pub screen_width: u8,
    pub screen_height: u8,

    pub lora_pin_sck: u8,
    pub lora_pin_miso: u8,
    pub lora_pin_mosi: u8,
    pub lora_pin_cs: u8,
    pub lora_pin_rst: u8,
    pub lora_pin_irq: u8,

    pub lora_frequency: f32,
    pub lora_bw: f32,
    pub lora_sf: u8,
    pub lora_cr: u8,
    pub lora_sync: u8,
    pub lora_preamble: u8,
    pub lora_tx_power: u8,
    pub lora_crc: bool,
    pub lora_afc: bool,
    pub lora_afc_bandwidth: f32,

    pub tx_interval: u32,
    pub rx_timeout: u32,

    pub wifi_ssid: [u8; 64],
    pub wifi_password: [u8; 64],
    pub wifi_enabled: bool,
    pub wifi_hostname: [u8; 32],
}

// Helper to create padded byte array from string literal
const fn make_padded_array<const N: usize>(s: &[u8]) -> [u8; N] {
    let mut arr = [0u8; N];
    let mut i = 0;
    while i < s.len() && i < N {
        arr[i] = s[i];
        i += 1;
    }
    arr
}

// Default values
impl Default for Config {
    fn default() -> Self {
        Self {
            node_id: 1,
            network_id: 1,
            capabilities: 0x03,
            user: make_padded_array(b"Benni3"),

            oled_pin_sda: 21,
            oled_pin_scl: 22,
            screen_width: 128,
            screen_height: 64,

            lora_pin_sck: 5,
            lora_pin_miso: 19,
            lora_pin_mosi: 27,
            lora_pin_cs: 18,
            lora_pin_rst: 23,
            lora_pin_irq: 26,

            lora_frequency: 868.0,
            lora_bw: 125.0,
            lora_sf: 9,
            lora_cr: 7,
            lora_sync: 0x12,
            lora_preamble: 8,
            lora_tx_power: 10,
            lora_crc: true,
            lora_afc: true,
            lora_afc_bandwidth: 125.0,

            tx_interval: 3000,
            rx_timeout: 10000,

            wifi_ssid: [0; 64],
            wifi_password: [0; 64],
            wifi_enabled: true,
            wifi_hostname: make_padded_array(b"lama"),
        }
    }
}

// Global config variable
static mut CONFIG: Config = Config {
    node_id: 1,
    network_id: 1,
    capabilities: 0x03,
    user: make_padded_array(b"Benni3"),
    oled_pin_sda: 21,
    oled_pin_scl: 22,
    screen_width: 128,
    screen_height: 64,
    lora_pin_sck: 5,
    lora_pin_miso: 19,
    lora_pin_mosi: 27,
    lora_pin_cs: 18,
    lora_pin_rst: 23,
    lora_pin_irq: 26,
    lora_frequency: 868.0,
    lora_bw: 125.0,
    lora_sf: 9,
    lora_cr: 7,
    lora_sync: 0x12,
    lora_preamble: 8,
    lora_tx_power: 10,
    lora_crc: true,
    lora_afc: true,
    lora_afc_bandwidth: 125.0,
    tx_interval: 3000,
    rx_timeout: 10000,
    wifi_ssid: [0; 64],
    wifi_password: [0; 64],
    wifi_enabled: true,
    wifi_hostname: make_padded_array(b"lama"),
};

// FFI-safe getters
pub fn get_config() -> &'static Config {
    unsafe { &*core::ptr::addr_of!(CONFIG) }
}

pub fn get_config_mut() -> &'static mut Config {
    unsafe { &mut *core::ptr::addr_of_mut!(CONFIG) }
}

// NVS functions removed - handled by Arduino Preferences API
//
// To load/save config from Arduino:
// 1. Use Arduino Preferences API to read from NVS
// 2. Call get_config_mut() to get Rust config pointer
// 3. Update fields from Arduino
//
// Example (Arduino C++):
//   Config* cfg = get_config_mut();
//   preferences.begin("lora", false);
//   cfg->node_id = preferences.getUChar("nodeId", 1);
//   preferences.end();
//
// This approach keeps NVS handling in Arduino where it belongs
