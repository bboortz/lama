/*
todos
* packet handler
* command handler
*/
#include <SPI.h>
#include <Wire.h>

#include <cstdint>

#include "config.h"
#include "display.h"
#include "lora.h"
#include "lora_packet.h"
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
  displayStatus();
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
      checkLoraState();

      break;

    case ERROR:
      Serial.printf("ERROR! Error code: %d\n", systemState);
      displayStatus();

      checkLoraState();

      break;

    case FATAL:
      Serial.printf("FATAL! Error code: %d\n", systemState);
      displayStatus();
      while (true) {
        ;
      }
  }
}
