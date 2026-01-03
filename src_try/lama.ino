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
#include "lora_stats.h"
#include "nodes.h"
#include "serial.h"
#include "state.h"
#include "wifi.h"
#include "lama_packet.h"

/*

  // Validate packet
  uint8_t data[64];
  size_t len = radio->readData(data, 64);
  if (validate_lora_packet(data, len)) {
      // Packet is valid
  }

*/

#define PROG_NAME "LAMA"
#define PROG_VER "0.1.0"

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
  // Test FFI
  bool works = lamaPacketCallable();  // Should return true

  Serial.println(works);
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
