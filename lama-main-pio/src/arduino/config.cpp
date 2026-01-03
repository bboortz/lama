#include "config.h"

#include <Preferences.h>

Preferences preferences;

// Define the global config variable (storage allocated here)
/*
Config config = {
    .nodeId        = DEFAULT_NODEID,
    .networkId     = DEFAULT_NETWORKID,
    .capabilities  = DEFAULT_CAPABILITIES,
    .user          = DEFAULT_USER,
    .oledPinSda    = DEFAULT_OLED_PIN_SDA,
    .oledPinScl    = DEFAULT_OLED_PIN_SCL,
    .screenWidth   = DEFAULT_SCREEN_WIDTH,
    .screenHeight  = DEFAULT_SCREEN_HEIGHT,
    .loraPinSck    = DEFAULT_LORA_PIN_SCK,
    .loraPinMiso   = DEFAULT_LORA_PIN_MISO,
    .loraPinMosi   = DEFAULT_LORA_PIN_MOSI,
    .loraPinCs     = DEFAULT_LORA_PIN_CS,
    .loraPinRst    = DEFAULT_LORA_PIN_RST,
    .loraPinIrq    = DEFAULT_LORA_PIN_IRQ,
    .loraFrequency = DEFAULT_LORA_FREQ,
    .loraBw        = DEFAULT_LORA_BW,
    .loraSf        = DEFAULT_LORA_SF,
    .loraCr        = DEFAULT_LORA_CR,
    .loraSync      = DEFAULT_LORA_SYNC,
    .loraPreamble  = DEFAULT_LORA_PREAMBLE,
    .loraTxPower   = DEFAULT_LORA_TXPOWER,
    .loraCrc       = DEFAULT_LORA_CRC,
    .loraAfc       = DEFAULT_LORA_AFC,
    .loraAfcBandwidth = DEFAULT_LORA_AFC_BANDWIDTH,
    .txInterval    = DEFAULT_TX_INTERVAL,
    .rxTimeout     = DEFAULT_RX_TIMEOUT,
    .wifiEnabled   = DEFAULT_WIFI_ENABLED,
    .wifiHostname  = DEFAULT_WIFI_HOSTNAME,
    .wifiSsid  = DEFAULT_WIFI_SSID,
    .wifiPassword = DEFAULT_WIFI_PASSWORD
};
*/

Config config = {
    DEFAULT_NODEID,         // nodeId
    DEFAULT_NETWORKID,      // networkId
    DEFAULT_CAPABILITIES,   // capabilities
    DEFAULT_USER,           // user
    DEFAULT_OLED_PIN_SDA,   // oledPinSda
    DEFAULT_OLED_PIN_SCL,   // oledPinScl
    DEFAULT_SCREEN_WIDTH,   // screenWidth
    DEFAULT_SCREEN_HEIGHT,  // screenHeight
    DEFAULT_LORA_PIN_SCK,   // loraPinSck
    DEFAULT_LORA_PIN_MISO,  // loraPinMiso
    DEFAULT_LORA_PIN_MOSI,  // loraPinMosi
    DEFAULT_LORA_PIN_CS,    // loraPinCs
    DEFAULT_LORA_PIN_RST,   // loraPinRst
    DEFAULT_LORA_PIN_IRQ,   // loraPinIrq
    868.0,                  // loraFrequency
    125.0,                  // loraBw
    9,                      // loraSf
    7,                      // loraCr
    0x12,                   // loraSync
    8,                      // loraPreamble
    10,                     // loraTxPower
    true,                   // loraCrc
    true,                   // loraAfc
    125.0,                  // loraAfcBandwidth
    3000,                   // txInterval
    10000,                  // rxTimeout
    "",                     // wifiSsid
    "",                     // wifiPassword
    true,                   // wifiEnabled
    "lama"                  // wifiHos
};

void loadConfig() {
  preferences.begin("lora", true);  // true = read-only

  // Node settings
  config.nodeId       = preferences.getUChar("nodeId", DEFAULT_NODEID);
  config.networkId    = preferences.getUChar("networkId", DEFAULT_NETWORKID);
  config.capabilities = preferences.getUChar("capabilities", DEFAULT_CAPABILITIES);
  String savedUser    = preferences.getString("user", DEFAULT_USER);
  savedUser.toCharArray(config.user, sizeof(config.user));

  // OLED settings
  config.oledPinSda   = preferences.getInt("oledPinSda", DEFAULT_OLED_PIN_SDA);
  config.oledPinScl   = preferences.getInt("oledPinScl", DEFAULT_OLED_PIN_SCL);
  config.screenWidth  = preferences.getInt("screenWidth", DEFAULT_SCREEN_WIDTH);
  config.screenHeight = preferences.getInt("screenHeight", DEFAULT_SCREEN_HEIGHT);

  // LoRa pin configuration
  config.loraPinSck  = preferences.getInt("loraPinSck", DEFAULT_LORA_PIN_SCK);
  config.loraPinMiso = preferences.getInt("loraPinMiso", DEFAULT_LORA_PIN_MISO);
  config.loraPinMosi = preferences.getInt("loraPinMosi", DEFAULT_LORA_PIN_MOSI);
  config.loraPinCs   = preferences.getInt("loraPinCs", DEFAULT_LORA_PIN_CS);
  config.loraPinRst  = preferences.getInt("loraPinRst", DEFAULT_LORA_PIN_RST);  // FIXED!
  config.loraPinIrq  = preferences.getInt("loraPinIrq", DEFAULT_LORA_PIN_IRQ);

  // LoRa RF parameters
  config.loraFrequency    = preferences.getFloat("loraFrequency", DEFAULT_LORA_FREQ);
  config.loraBw           = preferences.getFloat("loraBw", DEFAULT_LORA_BW);
  config.loraSf           = preferences.getInt("loraSf", DEFAULT_LORA_SF);
  config.loraCr           = preferences.getInt("loraCr", DEFAULT_LORA_CR);
  config.loraSync         = preferences.getInt("loraSync", DEFAULT_LORA_SYNC);
  config.loraPreamble     = preferences.getInt("loraPreamble", DEFAULT_LORA_PREAMBLE);
  config.loraTxPower      = preferences.getInt("loraTxPower", DEFAULT_LORA_TXPOWER);
  config.loraCrc          = preferences.getInt("loraCrc", DEFAULT_LORA_CRC);
  config.loraAfc          = preferences.getInt("loraAfc", DEFAULT_LORA_AFC);
  config.loraAfcBandwidth = preferences.getFloat("loraAfcBw", DEFAULT_LORA_AFC_BANDWIDTH);

  // Timing
  config.txInterval = preferences.getInt("txInterval", DEFAULT_TX_INTERVAL);
  config.rxTimeout  = preferences.getInt("rxTimeout", DEFAULT_RX_TIMEOUT);

  // WiFi settings
  String savedSsid = preferences.getString("wifiSsid", "");
  savedSsid.toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  String savedPassword = preferences.getString("wifiPassword", "");
  savedPassword.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
  config.wifiEnabled   = preferences.getBool("wifiEnabled", true);
  String savedHostname = preferences.getString("wifiHostname", "lama");
  savedHostname.toCharArray(config.wifiHostname, sizeof(config.wifiHostname));

  preferences.end();

  Serial.println("Config loaded");
}

void saveConfig() {
  preferences.begin("lora", false);  // false = read-write

  // Node settings
  preferences.putUChar("nodeId", config.nodeId);
  preferences.putUChar("networkId", config.networkId);
  preferences.putUChar("capabilities", config.capabilities);
  preferences.putString("user", String(config.user));

  // OLED settings
  preferences.putInt("oledPinSda", config.oledPinSda);
  preferences.putInt("oledPinScl", config.oledPinScl);
  preferences.putInt("screenWidth", config.screenWidth);
  preferences.putInt("screenHeight", config.screenHeight);

  // LoRa pin configuration
  preferences.putInt("loraPinSck", config.loraPinSck);
  preferences.putInt("loraPinMiso", config.loraPinMiso);
  preferences.putInt("loraPinMosi", config.loraPinMosi);
  preferences.putInt("loraPinCs", config.loraPinCs);
  preferences.putInt("loraPinRst", config.loraPinRst);
  preferences.putInt("loraPinIrq", config.loraPinIrq);

  // LoRa RF parameters
  preferences.putFloat("loraFrequency", config.loraFrequency);
  preferences.putFloat("loraBw", config.loraBw);
  preferences.putInt("loraSf", config.loraSf);
  preferences.putInt("loraCr", config.loraCr);
  preferences.putInt("loraSync", config.loraSync);
  preferences.putInt("loraPreamble", config.loraPreamble);
  preferences.putInt("loraTxPower", config.loraTxPower);
  preferences.putInt("loraCrc", config.loraCrc);
  preferences.putInt("loraAfc", config.loraAfc);
  preferences.putFloat("loraAfcBw", config.loraAfcBandwidth);

  // Timing
  preferences.putInt("txInterval", config.txInterval);
  preferences.putInt("rxTimeout", config.rxTimeout);

  // WiFi settings
  preferences.putString("wifiSsid", String(config.wifiSsid));
  preferences.putString("wifiPassword", String(config.wifiPassword));
  preferences.putBool("wifiEnabled", config.wifiEnabled);
  preferences.putString("wifiHostname", String(config.wifiHostname));

  preferences.end();

  Serial.println("Config saved!");
}

void resetConfig() {
  preferences.begin("lora", false);
  preferences.clear();
  preferences.end();

  Serial.println("Config reset to defaults!");
  loadConfig();  // Reload defaults
}

void printConfig() {
  Serial.println("=== Current Config ===");
  Serial.println("Node Settings:");
  Serial.printf("  Node ID:        %d\n", config.nodeId);
  Serial.printf("  Network ID:     %d\n", config.networkId);
  Serial.printf("  Capabilities:   0x%02X\n", config.capabilities);
  Serial.printf("  User:           %s\n", config.user);

  Serial.println("\nOLED Display:");
  Serial.printf("  SDA Pin:        %d\n", config.oledPinSda);
  Serial.printf("  SCL Pin:        %d\n", config.oledPinScl);
  Serial.printf("  Resolution:     %dx%d\n", config.screenWidth, config.screenHeight);

  Serial.println("\nLoRa Pin Configuration:");
  Serial.printf("  SCK:            %d\n", config.loraPinSck);
  Serial.printf("  MISO:           %d\n", config.loraPinMiso);
  Serial.printf("  MOSI:           %d\n", config.loraPinMosi);
  Serial.printf("  CS:             %d\n", config.loraPinCs);
  Serial.printf("  RST:            %d\n", config.loraPinRst);
  Serial.printf("  IRQ:            %d\n", config.loraPinIrq);

  Serial.println("\nLoRa RF Parameters:");
  Serial.printf("  Frequency:      %.1f MHz\n", config.loraFrequency);
  Serial.printf("  Bandwidth:      %.1f kHz\n", config.loraBw);
  Serial.printf("  SF:             %d\n", config.loraSf);
  Serial.printf("  CR:             4/%d\n", config.loraCr);
  Serial.printf("  Sync Word:      0x%02X\n", config.loraSync);
  Serial.printf("  Preamble:       %d\n", config.loraPreamble);
  Serial.printf("  TX Power:       %d dBm\n", config.loraTxPower);
  Serial.printf("  CRC:            %s\n", config.loraCrc ? "ON" : "OFF");
  Serial.printf("  AFC:            %s\n", config.loraAfc ? "ON" : "OFF");
  Serial.printf("  AFC Bandwidth:  %.1f kHz\n", config.loraAfcBandwidth);

  Serial.println("\nTiming:");
  Serial.printf("  TX Interval:    %d ms\n", config.txInterval);
  Serial.printf("  RX Timeout:     %d ms\n", config.rxTimeout);

  Serial.println("\nWiFi:");
  Serial.printf("  Enabled:        %s\n", config.wifiEnabled ? "YES" : "NO");
  Serial.printf("  SSID:           %s\n",
                strlen(config.wifiSsid) > 0 ? config.wifiSsid : "(not set)");
  Serial.printf("  Password:       %s\n",
                strlen(config.wifiPassword) > 0 ? "********" : "(not set)");
  Serial.printf("  Hostname:       %s\n", config.wifiHostname);

  /*
  Serial.printf("  Status:         %s\n", getWifiStatus());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  IP Address:     %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Signal:         %d dBm\n", WiFi.RSSI());
  }
  */

  Serial.println("\nMemory:");
  Serial.printf("  Free Heap:      %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Largest Block:  %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  Serial.printf("  Min Free Heap:  %d bytes\n", ESP.getMinFreeHeap());

  Serial.println("======================");
}
