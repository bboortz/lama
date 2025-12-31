#include <Preferences.h>

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


Preferences preferences;


void loadConfig() {
  preferences.begin("lora", true);  // true = read-only
  
  // Load with defaults if not set
  config.nodeId = preferences.getUChar("nodeId", DEFAULT_NODEID);
  config.networkId = preferences.getUChar("networkId", DEFAULT_NETWORKID);
  config.capabilities = preferences.getUChar("capabilities", DEFAULT_CAPABILITIES);
  String savedUser = preferences.getString("user", DEFAULT_USER);
  savedUser.toCharArray(config.user, sizeof(config.user));
  
  config.loraFrequency = preferences.getFloat("loraFrequency", DEFAULT_LORA_FREQ);
  config.loraBw = preferences.getInt("loraBw", DEFAULT_LORA_BW);
  config.loraSf = preferences.getInt("loraSf", DEFAULT_LORA_SF);
  config.loraCr = preferences.getInt("loraCr", DEFAULT_LORA_CR);
  config.loraSync = preferences.getInt("loraSync", DEFAULT_LORA_SYNC);
  config.loraPreamble = preferences.getInt("loraPreamble", DEFAULT_LORA_PREAMBLE);
  config.loraTxPower = preferences.getInt("loraTxPower", DEFAULT_LORA_TXPOWER);
  config.loraCrc = preferences.getInt("loraCrc", DEFAULT_LORA_CRC);
  config.loraAfc = preferences.getInt("loraAfc", DEFAULT_LORA_AFC);
  config.loraAfcBandwidth = preferences.getInt("loraAfcBandwidth", DEFAULT_LORA_AFC_BANDWIDTH);
  config.txInterval = preferences.getInt("txInterval", DEFAULT_TX_INTERVAL);
  config.rxTimeout = preferences.getInt("rxTimeout", DEFAULT_RX_TIMEOUT);
  
  preferences.end();
  
  Serial.println("Config loaded:");  
}

void saveConfig() {
  preferences.begin("lora", false);  // false = read-write

  preferences.putUChar("nodeId", config.nodeId);
  preferences.putUChar("networkId", config.networkId);
  preferences.putUChar("capabilities", config.capabilities); 
  preferences.putString("user", String(config.user));
  preferences.putFloat("loraFrequency", config.loraFrequency);
  preferences.putInt("loraBw", config.loraBw);
  preferences.putInt("loraSf", config.loraSf);
  preferences.putInt("loraCr", config.loraCr);
  preferences.putInt("loraSync", config.loraSync);
  preferences.putInt("loraPreamble", config.loraPreamble);
  preferences.putInt("loraTxPower", config.loraTxPower);
  preferences.putInt("loraCrc", config.loraCrc);
  preferences.putInt("loraAfc", config.loraAfc);
  preferences.putInt("loraAfcBandwidth", config.loraAfcBandwidth);
  preferences.putInt("txInterval", config.txInterval);
  preferences.putInt("rxTimeout", config.rxTimeout);
  
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
  Serial.printf("  User:           %s\n", config.user);
  Serial.printf("  Frequency:      %.1f MHz\n", config.loraFrequency);
  Serial.printf("  Bandwidth:      %d kHz\n", config.loraBw);
  Serial.printf("  SF:             %d\n", config.loraSf);
  Serial.printf("  CR:             4/%d\n", config.loraCr);
  Serial.printf("  Sync Word:      0x%02X\n", config.loraSync);
  Serial.printf("  Preamble:       %d\n", config.loraPreamble);
  Serial.printf("  TX Power:       %d dBm\n", config.loraTxPower);
  Serial.printf("  CRC:            %s\n", config.loraCrc ? "ON" : "OFF");
  Serial.printf("  AFC:            %s\n", config.loraAfc ? "ON" : "OFF");
  Serial.printf("  AFC Bandwidth:  %d kHz\n", config.loraAfcBandwidth);
  Serial.printf("  TX Interval:    %d ms\n", config.txInterval);
  Serial.printf("  RX Timeout:     %d ms\n", config.rxTimeout);
  Serial.println("======================");
}

void handleSerialCommand() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  
  if (cmd.length() == 0) return;
  
  // Parse command and value
  int spaceIdx = cmd.indexOf(' ');
  String command = (spaceIdx > 0) ? cmd.substring(0, spaceIdx) : cmd;
  String value = (spaceIdx > 0) ? cmd.substring(spaceIdx + 1) : "";
  
  command.toLowerCase();
  
  if (command == "user" && value.length() > 0) {
    value.toCharArray(config.user, sizeof(config.user));
    saveConfig();
    Serial.printf("User set to: %s\n", config.user);
    
  } else if (command == "freq" && value.length() > 0) {
    config.loraFrequency = value.toFloat();
    saveConfig();
    Serial.printf("Frequency set to: %.1f MHz (reboot required)\n", config.loraFrequency);
    
  } else if (command == "bw" && value.length() > 0) {
    config.loraBw = value.toInt();
    saveConfig();
    Serial.printf("Bandwidth set to: %d kHz (reboot required)\n", config.loraBw);
    
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
    config.loraPreamble = value.toInt();
    saveConfig();
    Serial.printf("Preamble set to: %d (reboot required)\n", config.loraPreamble);
    
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
    config.loraAfcBandwidth = value.toInt();
    saveConfig();
    Serial.printf("AFC Bandwidth set to: %d kHz (reboot required)\n", config.loraAfcBandwidth);
    
  } else if (command == "txint" && value.length() > 0) {
    config.txInterval = value.toInt();
    saveConfig();
    Serial.printf("TX Interval set to: %d ms\n", config.txInterval);
    
  } else if (command == "rxtimeout" && value.length() > 0) {
    config.rxTimeout = value.toInt();
    saveConfig();
    Serial.printf("RX Timeout set to: %d ms\n", config.rxTimeout);
    
  } else if (command == "show" || command == "config") {
    printConfig();
    
  } else if (command == "reset") {
    Serial.println("Resetting config to defaults...");
    resetConfig();
    printConfig();
    
  } else if (command == "reboot" || command == "restart") {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();

  } else if (command == "users" || command == "stats") {
  printUserStats();

  } else if (command == "nodeid" && value.length() > 0) {
  config.nodeId = value.toInt();
  saveConfig();
  Serial.printf("Node ID set to: %d (reboot required)\n", config.nodeId);

  } else if (command == "netid" && value.length() > 0) {
    config.networkId = value.toInt();
    saveConfig();
    Serial.printf("Network ID set to: %d (reboot required)\n", config.networkId);

  } else if (command == "ping" && value.length() > 0) {
    uint8_t dst = value.toInt();
    sendPing(dst);

  } else if (command == "discover") {
    sendDiscover();

  } else if (command == "announce") {
    sendAnnounce();

  } else if (command == "nodes") {
    printNodes();
    
  } else if (command == "help" || command == "?") {
    Serial.println("=== Commands ===");
    Serial.println("  user <name>      - Set username");
    Serial.println("  freq <MHz>       - Set frequency (e.g. 868.0)");
    Serial.println("  bw <kHz>         - Set bandwidth (125, 250, 500)");
    Serial.println("  sf <7-12>        - Set spreading factor");
    Serial.println("  cr <5-8>         - Set coding rate (4/x)");
    Serial.println("  sync <hex>       - Set sync word (e.g. 12 or 34)");
    Serial.println("  preamble <n>     - Set preamble length");
    Serial.println("  power <2-20>     - Set TX power in dBm");
    Serial.println("  crc [on|off]     - Toggle or set CRC");
    Serial.println("  afc [on|off]     - Toggle or set AFC");
    Serial.println("  afcbw <kHz>      - Set AFC bandwidth");
    Serial.println("  txint <ms>       - Set TX interval");
    Serial.println("  rxtimeout <ms>   - Set RX timeout");
    Serial.println("  show             - Show current config");
    Serial.println("  stats            - Show the statistics");
    Serial.println("  reset            - Reset to defaults");
    Serial.println("  reboot           - Restart device");
    Serial.println("  help             - Show this help");
    Serial.println("================");
    
  } else {
    Serial.printf("Unknown command: %s (type 'help' for commands)\n", command.c_str());
  }
}