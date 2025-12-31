#ifndef CONFIG_H
#define CONFIG_H


// LILYGO T3 V1.6.x pin definitions
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 23
#define LORA_IRQ 26

// OLED
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// default config
#define DEFAULT_NODEID 1
#define DEFAULT_NETWORKID 1
#define DEFAULT_CAPABILITIES CAPA_RX | CAPA_TX
#define DEFAULT_USER "Benni2"
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
#define DEFAULT_TX_INTERVAL 3000  // 5 seconds
#define DEFAULT_RX_TIMEOUT 10000  // 10 seconds without RX = lost


// Config structure
struct Config {
  uint8_t nodeId;           
  uint8_t networkId;        
  uint8_t capabilities;
  char user[16];
  float loraFrequency;
  float loraBw;
  int loraSf;
  int loraCr;
  int loraSync;
  int loraPreamble;
  int loraTxPower;
  bool loraCrc;
  bool loraAfc;
  float loraAfcBandwidth;
  int txInterval;
  int rxTimeout;
};

Config config;


#endif