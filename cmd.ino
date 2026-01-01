
// ========== Command Handlers ==========

// Node Configuration
void cmdNodeId(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int id = value.toInt();
  if (id >= 0 && id <= 255) {
    config.nodeId = (uint8_t)id;
    saveConfig();
    Serial.printf("Node ID set to: %d (reboot required)\n", config.nodeId);
  } else {
    Serial.println("Error: Node ID must be 0-255");
  }
}

void cmdNetId(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int id = value.toInt();
  if (id >= 0 && id <= 255) {
    config.networkId = (uint8_t)id;
    saveConfig();
    Serial.printf("Network ID set to: %d (reboot required)\n", config.networkId);
  } else {
    Serial.println("Error: Network ID must be 0-255");
  }
}

void cmdCapabilities(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  config.capabilities = (uint8_t)strtol(value.c_str(), NULL, 16);
  saveConfig();
  Serial.printf("Capabilities set to: 0x%02X (reboot required)\n", config.capabilities);
}

void cmdUser(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  if (value.length() < sizeof(config.user)) {
    value.toCharArray(config.user, sizeof(config.user));
    saveConfig();
    Serial.printf("User set to: %s\n", config.user);
  } else {
    Serial.printf("Error: Username too long (max %d chars)\n", sizeof(config.user) - 1);
  }
}

// OLED Pin Configuration
void cmdOledSda(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.oledPinSda = pin;
    saveConfig();
    Serial.printf("OLED SDA pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdOledScl(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.oledPinScl = pin;
    saveConfig();
    Serial.printf("OLED SCL pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

// LoRa Pin Configuration
void cmdLoraSck(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinSck = pin;
    saveConfig();
    Serial.printf("LoRa SCK pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdLoraMiso(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinMiso = pin;
    saveConfig();
    Serial.printf("LoRa MISO pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdLoraMosi(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinMosi = pin;
    saveConfig();
    Serial.printf("LoRa MOSI pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdLoraCs(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinCs = pin;
    saveConfig();
    Serial.printf("LoRa CS pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdLoraRst(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinRst = pin;
    saveConfig();
    Serial.printf("LoRa RST pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

void cmdLoraIrq(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pin = value.toInt();
  if (pin >= 0 && pin <= 39) {
    config.loraPinIrq = pin;
    saveConfig();
    Serial.printf("LoRa IRQ pin set to: %d (reboot required)\n", pin);
  } else {
    Serial.println("Error: Invalid pin number (0-39)");
  }
}

// LoRa RF Parameters
void cmdFrequency(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  float freq = value.toFloat();
  if (freq >= 137.0 && freq <= 1020.0) {
    config.loraFrequency = freq;
    saveConfig();
    Serial.printf("Frequency set to: %.1f MHz (reboot required)\n", config.loraFrequency);
  } else {
    Serial.println("Error: Frequency must be 137-1020 MHz");
  }
}

void cmdBandwidth(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  float bw = value.toFloat();
  if (bw == 7.800 || bw == 10.400 || bw == 15.600 || bw == 20.800 || bw == 31.250 || bw == 41.700
      || bw == 62.500 || bw == 125.000 || bw == 250.000 || bw == 500.000) {
    config.loraBw = bw;
    saveConfig();
    Serial.printf("Bandwidth set to: %f kHz (reboot required)\n", config.loraBw);
  } else {
    Serial.println("Error: Invalid bandwidth (use 125, 250, 500 for kHz)");
  }
}

void cmdSpreadingFactor(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int sf = value.toInt();
  if (sf >= 7 && sf <= 12) {
    config.loraSf = sf;
    saveConfig();
    Serial.printf("SF set to: %d (reboot required)\n", config.loraSf);
  } else {
    Serial.println("Error: SF must be 7-12");
  }
}

void cmdCodingRate(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int cr = value.toInt();
  if (cr >= 5 && cr <= 8) {
    config.loraCr = cr;
    saveConfig();
    Serial.printf("CR set to: 4/%d (reboot required)\n", config.loraCr);
  } else {
    Serial.println("Error: CR must be 5-8");
  }
}

void cmdSyncWord(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  config.loraSync = (int)strtol(value.c_str(), NULL, 16);
  saveConfig();
  Serial.printf("Sync word set to: 0x%02X (reboot required)\n", config.loraSync);
}

void cmdPreamble(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int preamble = value.toInt();
  if (preamble >= 6 && preamble <= 65535) {
    config.loraPreamble = preamble;
    saveConfig();
    Serial.printf("Preamble set to: %d (reboot required)\n", config.loraPreamble);
  } else {
    Serial.println("Error: Preamble must be 6-65535");
  }
}

void cmdTxPower(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int pwr = value.toInt();
  if (pwr >= 2 && pwr <= 20) {
    config.loraTxPower = pwr;
    saveConfig();
    Serial.printf("TX Power set to: %d dBm (reboot required)\n", config.loraTxPower);
  } else {
    Serial.println("Error: Power must be 2-20 dBm");
  }
}

void cmdCrc(String value) {
  if (value == "0" || value == "1") {
    config.loraCrc = value.toInt();
  } else {
    Serial.printf("Wrong parameter: %s. Try: 0 or 1\n", value);
    return;
  }
  saveConfig();
  Serial.printf("CRC set to: %s (reboot required)\n", config.loraCrc ? "ON" : "OFF");
}

void cmdAfc(String value) {
  if (value == "0" || value == "1") {
    config.loraAfc = value.toInt();
  } else {
    Serial.printf("Wrong parameter: %s. Try: 0 or 1\n", value);
    return;
  }
  saveConfig();
  Serial.printf("AFC set to: %s (reboot required)\n", config.loraAfc ? "ON" : "OFF");
}

void cmdAfcBandwidth(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  config.loraAfcBandwidth = value.toFloat();
  saveConfig();
  Serial.printf("AFC Bandwidth set to: %f kHz (reboot required)\n", config.loraAfcBandwidth);
}

// Timing
void cmdTxInterval(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int interval = value.toInt();
  if (interval >= 100 && interval <= 3600000) {
    config.txInterval = interval;
    saveConfig();
    Serial.printf("TX Interval set to: %d ms\n", config.txInterval);
  } else {
    Serial.println("Error: TX interval must be 100-3600000 ms");
  }
}

void cmdRxTimeout(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int timeout = value.toInt();
  if (timeout >= 100 && timeout <= 60000) {
    config.rxTimeout = timeout;
    saveConfig();
    Serial.printf("RX Timeout set to: %d ms\n", config.rxTimeout);
  } else {
    Serial.println("Error: RX timeout must be 100-60000 ms");
  }
}

// WiFi Configuration
void cmdSsid(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  if (value.length() < sizeof(config.wifiSsid)) {
    value.toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
    saveConfig();
    Serial.printf("WiFi SSID set to: %s\n", config.wifiSsid);
  } else {
    Serial.printf("Error: SSID too long (max %d chars)\n", sizeof(config.wifiSsid) - 1);
  }
}

void cmdPassword(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  if (value.length() < sizeof(config.wifiPassword)) {
    value.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
    saveConfig();
    Serial.println("WiFi password set (hidden)");
  } else {
    Serial.printf("Error: Password too long (max %d chars)\n", sizeof(config.wifiPassword) - 1);
  }
}

void cmdHostname(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  if (value.length() < sizeof(config.wifiHostname)) {
    value.toCharArray(config.wifiHostname, sizeof(config.wifiHostname));
    saveConfig();
    Serial.printf("WiFi hostname set to: %s (reboot required)\n", config.wifiHostname);
  } else {
    Serial.printf("Error: Hostname too long (max %d chars)\n", sizeof(config.wifiHostname) - 1);
  }
}

void cmdWifiEnable(String value) {
  if (value == "1" || value == "0") {
    config.wifiEnabled = bool(value.toInt());
    saveConfig();
    Serial.printf("WiFi enabled = %s (reboot required)\n", config.wifiEnabled ? "true" : "false");
  }
}

void cmdWifiCommand(String value) {
  if (value == "status") {
    Serial.println("WiFi Status:");
    Serial.printf("  Enabled:  %s\n", config.wifiEnabled ? "YES" : "NO");

    Serial.printf("  Status:   %s\n", getWifiStatus());
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("  SSID:     %s\n", WiFi.SSID().c_str());
      Serial.printf("  IP:       %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("  Signal:   %d dBm\n", WiFi.RSSI());
      Serial.printf("  Hostname: %s\n", WiFi.getHostname());
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
}

// Network Commands
/*
void cmdPing(String value) {
  if (value.length() == 0) {
    Serial.println("Error: Value required");
    return;
  }

  int dst = value.toInt();
  if (dst >= 0 && dst <= 255) {
    sendPing((uint8_t)dst);
    Serial.printf("Ping sent to node %d\n", dst);
  } else {
    Serial.println("Error: Node ID must be 0-255");
  }
}

void cmdDiscover(String value) {
  sendDiscover();
  Serial.println("Discovery broadcast sent");
}

void cmdAnnounce(String value) {
  sendAnnounce();
  Serial.println("Announce broadcast sent");
}
*/
/*
void cmdNodes(String value) {
  printNodes();
}
*/

// Statistics and Info
void cmdMemory(String value) {
  Serial.println("=== Memory Info ===");
  Serial.printf("Free Heap:        %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Min Free Heap:    %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("Largest Block:    %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  Serial.printf("Total Heap:       %d bytes\n", ESP.getHeapSize());
  Serial.printf("Flash Size:       %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Flash Speed:      %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
  Serial.printf("Chip Model:       %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision:    %d\n", ESP.getChipRevision());
  Serial.printf("CPU Frequency:    %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK Version:      %s\n", ESP.getSdkVersion());
  Serial.println("===================");
}

void cmdUptime(String value) {
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
}

// System Commands
void cmdReset(String value) {
  Serial.println("Resetting config to defaults...");
  resetConfig();
  printConfig();
  Serial.println("Note: Reboot required for changes to take effect");
}

void cmdReboot(String value) {
  Serial.println("Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

void cmdHelp(String value) {
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
  Serial.println("  wifiEnable [on|off| - Enable/disable WiFi");
  Serial.println("  wifiCommand [status - WiFi Status");
  Serial.println("        reconnect]    - WiFi Reconnect");
  Serial.println("       ");

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
}