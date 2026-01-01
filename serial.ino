
void setupSerial() {
  // serial setup
  Serial.begin(115200);
#if SERIAL_WAIT_TIMEOUT > 0
  unsigned long serialTimeout = millis() + SERIAL_WAIT_TIMEOUT;
  while (!Serial && millis() < serialTimeout) {
    delay(10);
  }
#elif SERIAL_WAIT_TIMEOUT < 0
  while (!Serial)
    ;  // Wait forever (debugging only)
#endif
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

  // Dispatch to handlers
  if (command == "nodeid") {
    cmdNodeId(value);
  } else if (command == "netid") {
    cmdNetId(value);
  } else if (command == "cap") {
    cmdCapabilities(value);
  } else if (command == "user") {
    cmdUser(value);
  } else if (command == "oled_sda") {
    cmdOledSda(value);
  } else if (command == "oled_scl") {
    cmdOledScl(value);
  } else if (command == "lora_sck") {
    cmdLoraSck(value);
  } else if (command == "lora_miso") {
    cmdLoraMiso(value);
  } else if (command == "lora_mosi") {
    cmdLoraMosi(value);
  } else if (command == "lora_cs") {
    cmdLoraCs(value);
  } else if (command == "lora_rst") {
    cmdLoraRst(value);
  } else if (command == "lora_irq") {
    cmdLoraIrq(value);
  } else if (command == "freq") {
    cmdFrequency(value);
  } else if (command == "bw") {
    cmdBandwidth(value);
  } else if (command == "sf") {
    cmdSpreadingFactor(value);
  } else if (command == "cr") {
    cmdCodingRate(value);
  } else if (command == "sync") {
    cmdSyncWord(value);
  } else if (command == "preamble") {
    cmdPreamble(value);
  } else if (command == "power") {
    cmdTxPower(value);
  } else if (command == "crc") {
    cmdCrc(value);
  } else if (command == "afc") {
    cmdAfc(value);
  } else if (command == "afcbw") {
    cmdAfcBandwidth(value);
  } else if (command == "txint") {
    cmdTxInterval(value);
  } else if (command == "rxtimeout") {
    cmdRxTimeout(value);
  } else if (command == "ssid") {
    cmdSsid(value);
  } else if (command == "password") {
    cmdPassword(value);
  } else if (command == "hostname") {
    cmdHostname(value);
  } else if (command == "wifiEnable") {
    cmdWifiEnable(value);
  } else if (command == "wifiCommand") {
    cmdWifiCommand(value);
    /*
    } else if (command == "ping") {
      cmdPing(value);
    } else if (command == "discover") {
      cmdDiscover(value);
    } else if (command == "announce") {
      cmdAnnounce(value);
      */
    /*
} else if (command == "nodes") {
  cmdNodes(value);
  */
  } else if (command == "users" || command == "stats") {
    printUserStats();
  } else if (command == "show" || command == "config") {
    printConfig();
  } else if (command == "mem" || command == "memory") {
    cmdMemory(value);
  } else if (command == "uptime") {
    cmdUptime(value);
  } else if (command == "reset") {
    cmdReset(value);
  } else if (command == "reboot" || command == "restart") {
    cmdReboot(value);
  } else if (command == "help" || command == "?") {
    cmdHelp(value);
  } else {
    Serial.printf("Unknown command: '%s' (type 'help' for commands)\n", command.c_str());
  }
}