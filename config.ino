#include <Preferences.h>

Preferences preferences;

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
  config.loraAfcBandwidth = preferences.getFloat("loraAfcBandwidth", DEFAULT_LORA_AFC_BANDWIDTH);

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
  preferences.putFloat("loraAfcBandwidth", config.loraAfcBandwidth);

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
  
  Serial.printf("  Status:         %s\n", getWifiStatus() );
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  IP Address:     %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Signal:         %d dBm\n", WiFi.RSSI());
  }

  Serial.println("\nMemory:");
  Serial.printf("  Free Heap:      %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Largest Block:  %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  Serial.printf("  Min Free Heap:  %d bytes\n", ESP.getMinFreeHeap());

  Serial.println("======================");
}

void handleSerialCommand() {
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) {
    return;
  }

  // Parse command and value
  int    spaceIdx = cmd.indexOf(' ');
  String command  = (spaceIdx > 0) ? cmd.substring(0, spaceIdx) : cmd;
  String value    = (spaceIdx > 0) ? cmd.substring(spaceIdx + 1) : "";

  command.toLowerCase();

  // ========== Node Configuration ==========
  if (command == "nodeid" && value.length() > 0) {
    uint8_t id = value.toInt();
    if (id >= 0 && id <= 255) {
      config.nodeId = id;
      saveConfig();
      Serial.printf("Node ID set to: %d (reboot required)\n", config.nodeId);
    } else {
      Serial.println("Error: Node ID must be 0-255");
    }

  } else if (command == "netid" && value.length() > 0) {
    uint8_t id = value.toInt();
    if (id >= 0 && id <= 255) {
      config.networkId = id;
      saveConfig();
      Serial.printf("Network ID set to: %d (reboot required)\n", config.networkId);
    } else {
      Serial.println("Error: Network ID must be 0-255");
    }

  } else if (command == "cap" && value.length() > 0) {
    config.capabilities = (uint8_t)strtol(value.c_str(), NULL, 16);
    saveConfig();
    Serial.printf("Capabilities set to: 0x%02X (reboot required)\n", config.capabilities);

  } else if (command == "user" && value.length() > 0) {
    if (value.length() < sizeof(config.user)) {
      value.toCharArray(config.user, sizeof(config.user));
      saveConfig();
      Serial.printf("User set to: %s\n", config.user);
    } else {
      Serial.printf("Error: Username too long (max %d chars)\n", sizeof(config.user) - 1);
    }

    // ========== OLED Pin Configuration ==========
  } else if (command == "oled_sda" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.oledPinSda = pin;
      saveConfig();
      Serial.printf("OLED SDA pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "oled_scl" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.oledPinScl = pin;
      saveConfig();
      Serial.printf("OLED SCL pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

    // ========== LoRa Pin Configuration ==========
  } else if (command == "lora_sck" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinSck = pin;
      saveConfig();
      Serial.printf("LoRa SCK pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "lora_miso" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinMiso = pin;
      saveConfig();
      Serial.printf("LoRa MISO pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "lora_mosi" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinMosi = pin;
      saveConfig();
      Serial.printf("LoRa MOSI pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "lora_cs" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinCs = pin;
      saveConfig();
      Serial.printf("LoRa CS pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "lora_rst" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinRst = pin;
      saveConfig();
      Serial.printf("LoRa RST pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

  } else if (command == "lora_irq" && value.length() > 0) {
    int pin = value.toInt();
    if (pin >= 0 && pin <= 39) {
      config.loraPinIrq = pin;
      saveConfig();
      Serial.printf("LoRa IRQ pin set to: %d (reboot required)\n", pin);
    } else {
      Serial.println("Error: Invalid pin number (0-39)");
    }

    // ========== LoRa RF Parameters ==========
  } else if (command == "freq" && value.length() > 0) {
    float freq = value.toFloat();
    if (freq >= 137.0 && freq <= 1020.0) {
      config.loraFrequency = freq;
      saveConfig();
      Serial.printf("Frequency set to: %.1f MHz (reboot required)\n", config.loraFrequency);
    } else {
      Serial.println("Error: Frequency must be 137-1020 MHz");
    }

  } else if (command == "bw" && value.length() > 0) {
    float bw = value.toFloat();
    if (bw == 7.800 || bw == 10.400 || bw == 15.600 || bw == 20.800 || bw == 31.250 || bw == 41.700
        || bw == 62.500 || bw == 125.000 || bw == 250.000 || bw == 500.000) {
      config.loraBw = bw;
      saveConfig();
      Serial.printf("Bandwidth set to: %f kHz (reboot required)\n", config.loraBw);
    } else {
      Serial.println("Error: Invalid bandwidth (use 125, 250, 500 for kHz)");
    }

  } else if (command == "sf" && value.length() > 0) {
    int sf = value.toInt();
    if (sf >= 7 && sf <= 12) {
      config.loraSf = sf;
      saveConfig();
      Serial.printf("SF set to: %d (reboot required)\n", config.loraSf);
    } else {
      Serial.println("Error: SF must be 7-12");
    }

  } else if (command == "cr" && value.length() > 0) {
    int cr = value.toInt();
    if (cr >= 5 && cr <= 8) {
      config.loraCr = cr;
      saveConfig();
      Serial.printf("CR set to: 4/%d (reboot required)\n", config.loraCr);
    } else {
      Serial.println("Error: CR must be 5-8");
    }

  } else if (command == "sync" && value.length() > 0) {
    config.loraSync = (int)strtol(value.c_str(), NULL, 16);
    saveConfig();
    Serial.printf("Sync word set to: 0x%02X (reboot required)\n", config.loraSync);

  } else if (command == "preamble" && value.length() > 0) {
    int preamble = value.toInt();
    if (preamble >= 6 && preamble <= 65535) {
      config.loraPreamble = preamble;
      saveConfig();
      Serial.printf("Preamble set to: %d (reboot required)\n", config.loraPreamble);
    } else {
      Serial.println("Error: Preamble must be 6-65535");
    }

  } else if (command == "power" && value.length() > 0) {
    int pwr = value.toInt();
    if (pwr >= 2 && pwr <= 20) {
      config.loraTxPower = pwr;
      saveConfig();
      Serial.printf("TX Power set to: %d dBm (reboot required)\n", config.loraTxPower);
    } else {
      Serial.println("Error: Power must be 2-20 dBm");
    }

  } else if (command == "crc") {
    if (value == "on" || value == "1") {
      config.loraCrc = 1;
    } else if (value == "off" || value == "0") {
      config.loraCrc = 0;
    } else {
      config.loraCrc = !config.loraCrc;  // Toggle
    }
    saveConfig();
    Serial.printf("CRC set to: %s (reboot required)\n", config.loraCrc ? "ON" : "OFF");

  } else if (command == "afc") {
    if (value == "on" || value == "1") {
      config.loraAfc = 1;
    } else if (value == "off" || value == "0") {
      config.loraAfc = 0;
    } else {
      config.loraAfc = !config.loraAfc;  // Toggle
    }
    saveConfig();
    Serial.printf("AFC set to: %s (reboot required)\n", config.loraAfc ? "ON" : "OFF");

  } else if (command == "afcbw" && value.length() > 0) {
    config.loraAfcBandwidth = value.toFloat();
    saveConfig();
    Serial.printf("AFC Bandwidth set to: %f kHz (reboot required)\n", config.loraAfcBandwidth);

    // ========== Timing ==========
  } else if (command == "txint" && value.length() > 0) {
    int interval = value.toInt();
    if (interval >= 100 && interval <= 3600000) {
      config.txInterval = interval;
      saveConfig();
      Serial.printf("TX Interval set to: %d ms\n", config.txInterval);
    } else {
      Serial.println("Error: TX interval must be 100-3600000 ms");
    }

  } else if (command == "rxtimeout" && value.length() > 0) {
    int timeout = value.toInt();
    if (timeout >= 100 && timeout <= 60000) {
      config.rxTimeout = timeout;
      saveConfig();
      Serial.printf("RX Timeout set to: %d ms\n", config.rxTimeout);
    } else {
      Serial.println("Error: RX timeout must be 100-60000 ms");
    }

    // ========== WiFi Configuration ==========
  } else if (command == "ssid" && value.length() > 0) {
    if (value.length() < sizeof(config.wifiSsid)) {
      value.toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
      saveConfig();
      Serial.printf("WiFi SSID set to: %s\n", config.wifiSsid);
    } else {
      Serial.printf("Error: SSID too long (max %d chars)\n", sizeof(config.wifiSsid) - 1);
    }

  } else if (command == "password" && value.length() > 0) {
    if (value.length() < sizeof(config.wifiPassword)) {
      value.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
      saveConfig();
      Serial.println("WiFi password set (hidden)");
    } else {
      Serial.printf("Error: Password too long (max %d chars)\n", sizeof(config.wifiPassword) - 1);
    }

  } else if (command == "hostname" && value.length() > 0) {
    if (value.length() < sizeof(config.wifiHostname)) {
      value.toCharArray(config.wifiHostname, sizeof(config.wifiHostname));
      saveConfig();
      Serial.printf("WiFi hostname set to: %s (reboot required)\n", config.wifiHostname);
    } else {
      Serial.printf("Error: Hostname too long (max %d chars)\n", sizeof(config.wifiHostname) - 1);
    }

  } else if (command == "wifi") {
    if (value == "on" || value == "enable" || value == "1") {
      config.wifiEnabled = true;
      saveConfig();
      Serial.println("WiFi enabled (reboot required)");
    } else if (value == "off" || value == "disable" || value == "0") {
      config.wifiEnabled = false;
      saveConfig();
      Serial.println("WiFi disabled (reboot required)");
    } else if (value == "status") {
      Serial.println("WiFi Status:");
      Serial.printf("  Enabled:  %s\n", config.wifiEnabled ? "YES" : "NO");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("  Status:   Connected\n");
        Serial.printf("  SSID:     %s\n", WiFi.SSID().c_str());
        Serial.printf("  IP:       %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("  Signal:   %d dBm\n", WiFi.RSSI());
        Serial.printf("  Hostname: %s\n", WiFi.getHostname());
      } else {
        Serial.printf("  Status:   Disconnected\n");
      }
    } else if (value == "reconnect") {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(config.wifiSsid, config.wifiPassword);
    } else {
      config.wifiEnabled = !config.wifiEnabled;
      saveConfig();
      Serial.printf("WiFi %s (reboot required)\n", config.wifiEnabled ? "enabled" : "disabled");
    }

    // ========== Network Commands ==========
  } else if (command == "ping" && value.length() > 0) {
    uint8_t dst = value.toInt();
    if (dst >= 0 && dst <= 255) {
      sendPing(dst);
      Serial.printf("Ping sent to node %d\n", dst);
    } else {
      Serial.println("Error: Node ID must be 0-255");
    }

  } else if (command == "discover") {
    sendDiscover();
    Serial.println("Discovery broadcast sent");

  } else if (command == "announce") {
    sendAnnounce();
    Serial.println("Announce broadcast sent");

  } else if (command == "nodes") {
    printNodes();

    // ========== Statistics and Info ==========
  } else if (command == "users" || command == "stats") {
    printUserStats();

  } else if (command == "show" || command == "config") {
    printConfig();

  } else if (command == "mem" || command == "memory") {
    Serial.println("=== Memory Info ===");
    Serial.printf("Free Heap:        %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min Free Heap:    %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Largest Block:    %d bytes\n",
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.printf("Total Heap:       %d bytes\n", ESP.getHeapSize());
    Serial.printf("Flash Size:       %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Flash Speed:      %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("Chip Model:       %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision:    %d\n", ESP.getChipRevision());
    Serial.printf("CPU Frequency:    %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("SDK Version:      %s\n", ESP.getSdkVersion());
    Serial.println("===================");

  } else if (command == "uptime") {
    unsigned long uptime  = millis();
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours   = minutes / 60;
    unsigned long days    = hours / 24;

    Serial.printf("Uptime: %lu days, %lu:%02lu:%02lu\n",
                  days,
                  hours % 24,
                  minutes % 60,
                  seconds % 60);

    // ========== System Commands ==========
  } else if (command == "reset") {
    Serial.println("Resetting config to defaults...");
    resetConfig();
    printConfig();
    Serial.println("Note: Reboot required for changes to take effect");

  } else if (command == "reboot" || command == "restart") {
    Serial.println("Rebooting in 1 second...");
    delay(1000);
    ESP.restart();

  } else if (command == "help" || command == "?") {
    Serial.println("=== LAMA Commands ===");
    Serial.println("\nNode Configuration:");
    Serial.println("  nodeid <0-255>      - Set node ID");
    Serial.println("  netid <0-255>       - Set network ID");
    Serial.println("  cap <hex>           - Set capabilities (e.g. FF)");
    Serial.println("  user <name>         - Set username");

    Serial.println("\nOLED Pin Configuration:");
    Serial.println("  oled_sda <pin>      - Set OLED SDA pin");
    Serial.println("  oled_scl <pin>      - Set OLED SCL pin");

    Serial.println("\nLoRa Pin Configuration:");
    Serial.println("  lora_sck <pin>      - Set LoRa SCK pin");
    Serial.println("  lora_miso <pin>     - Set LoRa MISO pin");
    Serial.println("  lora_mosi <pin>     - Set LoRa MOSI pin");
    Serial.println("  lora_cs <pin>       - Set LoRa CS pin");
    Serial.println("  lora_rst <pin>      - Set LoRa RST pin");
    Serial.println("  lora_irq <pin>      - Set LoRa IRQ pin");

    Serial.println("\nLoRa RF Parameters:");
    Serial.println("  freq <MHz>          - Set frequency (e.g. 868.0)");
    Serial.println("  bw <kHz>            - Set bandwidth (125, 250, 500)");
    Serial.println("  sf <7-12>           - Set spreading factor");
    Serial.println("  cr <5-8>            - Set coding rate (4/x)");
    Serial.println("  sync <hex>          - Set sync word (e.g. 12)");
    Serial.println("  preamble <n>        - Set preamble length");
    Serial.println("  power <2-20>        - Set TX power in dBm");
    Serial.println("  crc [on|off]        - Toggle or set CRC");
    Serial.println("  afc [on|off]        - Toggle or set AFC");
    Serial.println("  afcbw <kHz>         - Set AFC bandwidth");

    Serial.println("\nTiming:");
    Serial.println("  txint <ms>          - Set TX interval");
    Serial.println("  rxtimeout <ms>      - Set RX timeout");

    Serial.println("\nWiFi:");
    Serial.println("  ssid <name>         - Set WiFi SSID");
    Serial.println("  password <pass>     - Set WiFi password");
    Serial.println("  hostname <name>     - Set WiFi hostname");
    Serial.println("  wifi [on|off|       - Enable/disable WiFi");
    Serial.println("       status|reconnect]");

    Serial.println("\nNetwork:");
    Serial.println("  ping <nodeid>       - Ping specific node");
    Serial.println("  discover            - Send discovery broadcast");
    Serial.println("  announce            - Send announce broadcast");
    Serial.println("  nodes               - Show discovered nodes");

    Serial.println("\nInformation:");
    Serial.println("  show / config       - Show current configuration");
    Serial.println("  stats / users       - Show network statistics");
    Serial.println("  mem / memory        - Show memory information");
    Serial.println("  uptime              - Show system uptime");

    Serial.println("\nSystem:");
    Serial.println("  reset               - Reset to defaults");
    Serial.println("  reboot / restart    - Restart device");
    Serial.println("  help / ?            - Show this help");
    Serial.println("====================");

  } else {
    Serial.printf("Unknown command: '%s' (type 'help' for commands)\n", command.c_str());
  }
}