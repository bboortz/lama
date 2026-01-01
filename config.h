#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_NODEID 1
#define DEFAULT_NETWORKID 1
#define DEFAULT_CAPABILITIES (CAPA_RX | CAPA_TX)
#define DEFAULT_USER "Benni2"

// OLED
#define DEFAULT_OLED_PIN_SDA 21
#define DEFAULT_OLED_PIN_SCL 22
#define DEFAULT_SCREEN_WIDTH 128
#define DEFAULT_SCREEN_HEIGHT 64

// LILYGO T3 V1.6.x pin definitions
#define DEFAULT_LORA_PIN_SCK 5
#define DEFAULT_LORA_PIN_MISO 19
#define DEFAULT_LORA_PIN_MOSI 27
#define DEFAULT_LORA_PIN_CS 18
#define DEFAULT_LORA_PIN_RST 23
#define DEFAULT_LORA_PIN_IRQ 26

// lora config
#define DEFAULT_LORA_FREQ 868.0
#define DEFAULT_LORA_BW 125.0
#define DEFAULT_LORA_SF 9
#define DEFAULT_LORA_CR 7
#define DEFAULT_LORA_SYNC 0x12
#define DEFAULT_LORA_PREAMBLE 8
#define DEFAULT_LORA_TXPOWER 10
#define DEFAULT_LORA_CRC true
#define DEFAULT_LORA_AFC true
#define DEFAULT_LORA_AFC_BANDWIDTH 125.0

// rx tx
#define DEFAULT_TX_INTERVAL 3000  // 5 seconds
#define DEFAULT_RX_TIMEOUT 10000  // 10 seconds without RX = lost

// Wifi
// WiFi defaults
#define DEFAULT_WIFI_ENABLED true
#define DEFAULT_WIFI_HOSTNAME "lama"

// Config structure
struct Config {
  uint8_t nodeId;
  uint8_t networkId;
  uint8_t capabilities;
  char    user[16];

  uint8_t oledPinSda;
  uint8_t oledPinScl;
  uint8_t screenWidth;
  uint8_t screenHeight;

  uint8_t loraPinSck;
  uint8_t loraPinMiso;
  uint8_t loraPinMosi;
  uint8_t loraPinCs;
  uint8_t loraPinRst;
  uint8_t loraPinIrq;

  float loraFrequency;
  float loraBw;
  int   loraSf;
  int   loraCr;
  int   loraSync;
  int   loraPreamble;
  int   loraTxPower;
  bool  loraCrc;
  bool  loraAfc;
  float loraAfcBandwidth;

  int txInterval;
  int rxTimeout;

  char wifiSsid[64];
  char wifiPassword[64];
  bool wifiEnabled;
  char wifiHostname[32];
};

Config config;

#endif