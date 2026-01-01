

void setupLora() {
  // Init radiolib
  loraModule = new Module(config.loraPinCs, config.loraPinIrq, config.loraPinRst, RADIOLIB_NC);
  if (!loraModule) {
    displayError("LoRa module alloc failed!");
    return;
  }
  radio = new SX1276(loraModule);
  if (!radio) {
    displayError("Failed to allocate SX1276 radio!");
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
  if (doRadiolibState(radiolibState)) {
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

void enableRX(void) {
  radiolibState = radio->startReceive();
  if (doRadiolibState(radiolibState)) {
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


void handleRxError(int errorCode) {
  switch (errorCode) {
    case RADIOLIB_ERR_RX_TIMEOUT:
      Serial.println("RX Timeout!");
      break;
    case RADIOLIB_ERR_CRC_MISMATCH:
      Serial.println("CRC error!");
      break;
    default:
      Serial.printf("Receive failed, error: %d\n", errorCode);
      break;
  }
}

void receivePacket(void) {
  if (systemState != DO_RX) {
    return;
  }
  
  setStatus(IN_RX);
  decodeLoraPacket();

  enableRX();
  updateStatus();
  
  setStatus(IDLE);
}

void transmitPacket(void) {
  if (systemState != DO_TX) {
    return;
  }
  setStatus(IN_TX);
  txPacketCount++;
  lastTxTime = millis();

  // Build message
  int m     = millis() % 100;
  txMessage = String(config.user) + " " + padZeroRight(txPacketCount, 3) + " " + padZeroRight(m, 3)
              + " " + String(getLastSnr());

  txState = radio->transmit(txMessage);
  enableRX();
  updateStatus();

  setStatus(IDLE);
}
