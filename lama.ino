/*
todos
* packet
* command handler
*/
#include <SPI.h>
#include <Wire.h>

#include <cstdint>

#include "config.h"
#include "display.h"
#include "lora.h"
#include "lora_packet.h"
#include "packet.h"
#include "rx_history.h"
#include "serial.h"
#include "state.h"
#include "wifi.h"

void setupBoard() {
  currentMethod = "setup()";
  systemState   = INIT;
  delay(500);

  // Load config BEFORE radio setup
  loadConfig();

  setupSerial();
  delay(500);

  setupDisplay();
  delay(500);

  setupLora();
  delay(500);

  setupWifi();
  delay(500);

  // setup done
  setStatus(READY);
  delay(500);
}

void setup() {
  setupBoard();
}

void loop() {
  updateStatus();
  handleSerialCommand();
  handleWifi();

  switch (systemState) {
    case INIT:
      setupBoard();
      break;

    case READY:
      setStatus(IDLE);
      break;

    case DO_RX:
      receivePacket();
      break;

    case DO_TX:
      transmitPacket();
      break;

      //    case IN_RX:
      //    case IN_TX:

    case IDLE:
      if (lastRxTime > 0 && (millis() - lastRxTime > config.rxTimeout)) {
        // No packet received for 30 seconds
        setLoraNetworkState(LOST);
      }

      if (millis() - lastTxTime > config.txInterval) {
        setSystemState(DO_TX);
      }

      break;

    case FAILED:
      Serial.printf("Failed! Error code: %d\n", systemState);
      displayError("INIT FAILED!");
      while (true) {
        ;
      }
  }
}
