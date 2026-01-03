

void setupLora() {
  // Init radiolib
  loraModule = new Module(config.loraPinCs, config.loraPinIrq, config.loraPinRst, RADIOLIB_NC);
  if (!loraModule) {
    addError("LoRa module alloc failed!");
    setStatus(FATAL);
    return;
  }
  radio = new SX1276(loraModule);
  if (!radio) {
    addError("Failed to allocate SX1276 radio!");
    setStatus(FATAL);
    return;
  }

  // Init LORA SPI
  SPI.begin(config.loraPinSck, config.loraPinMiso, config.loraPinMosi, config.loraPinCs);

  // setup lora
  radiolibState = radio->begin(config.loraFrequency,
                               config.loraBw,
                               config.loraSf,
                               config.loraCr,
                               config.loraSync,
                               config.loraTxPower,  // output power (not used for RX)
                               config.loraPreamble);
  if (handleRadiolibState(radiolibState)) {
    addError("Lora setup error");
    setStatus(FATAL);
    return;
  }
  // Enable AFC (Automatic Frequency Correction)
  radio->setAFCBandwidth(config.loraAfcBandwidth);
  radio->setAFC(config.loraAfc);
  // Enable CRC
  radio->setCRC(config.loraCrc);

  // setup receiver
  radio->setPacketReceivedAction(onPacketRX);
  enableRX();
}

void checkLoraState() {
  if (lastRxTime > 0 && (millis() - lastRxTime > config.rxTimeout)) {
    // No packet received for 30 seconds
    setLoraNetworkState(LOST);
  }

  if (millis() - lastTxTime > config.txInterval) {
    setSystemState(DO_TX);
  }
}

void enableRX(void) {
  radiolibState = radio->startReceive();
  if (handleRadiolibState(radiolibState)) {
    addError("receiver error");
    setStatus(FATAL);
    return;
  }
}

// ISR - keep short!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onPacketRX(void) {
  setSystemState(DO_RX);
}

int handleRadiolibState(int state) {
  if (state != RADIOLIB_ERR_NONE) {
    setSystemState(ERROR);

    String msg;
    switch (state) {
      case RADIOLIB_ERR_RX_TIMEOUT:
        msg = "RX Timeout!";
        break;
      case RADIOLIB_ERR_CRC_MISMATCH:
        msg = "CRC error!";
        break;
      default:
        msg = "Receive failed, error: " + String(state);
        break;
    }
    addError(msg);
  }

  return state;
}

void receivePacket(void) {
  if (systemState != DO_RX) {
    return;
  }

  setStatus(IN_RX);
  decodeLoraPacket();

  enableRX();
  displayStatus();

  setStatus(IDLE);
}

void transmitPacket(void) {
  if (systemState != DO_TX) {
    return;
  }
  setStatus(IN_TX);
  loraTxPacketCount++;
  lastTxTime = millis();

  // Build message
  txState = sendLoraBeat();

  enableRX();
  displayStatus();

  setStatus(IDLE);
}
