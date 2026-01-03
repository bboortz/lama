  #ifndef LAMA_CORE_H
  #define LAMA_CORE_H

  #include <stdint.h>
  #include <stdbool.h>

  #ifdef __cplusplus
  extern "C" {
  #endif

  typedef struct {
      uint8_t node_id;
      uint8_t network_id;
      uint8_t capabilities;
      uint8_t user[16];

      uint8_t oled_pin_sda;
      uint8_t oled_pin_scl;
      uint8_t screen_width;
      uint8_t screen_height;

      uint8_t lora_pin_sck;
      uint8_t lora_pin_miso;
      uint8_t lora_pin_mosi;
      uint8_t lora_pin_cs;
      uint8_t lora_pin_rst;
      uint8_t lora_pin_irq;

      float lora_frequency;
      float lora_bw;
      uint8_t lora_sf;
      uint8_t lora_cr;
      uint8_t lora_sync;
      uint8_t lora_preamble;
      uint8_t lora_tx_power;
      bool lora_crc;
      bool lora_afc;
      float lora_afc_bandwidth;

      uint32_t tx_interval;
      uint32_t rx_timeout;

      uint8_t wifi_ssid[64];
      uint8_t wifi_password[64];
      bool wifi_enabled;
      uint8_t wifi_hostname[32];
  } Config;

  // Rust FFI functions
  const Config* get_config(void);
  Config* get_config_mut(void);
  bool lama_packet_callable(void);

  #ifdef __cplusplus
  }
  #endif
  #endif

