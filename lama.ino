
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"
#include "state.h"
#include "display.h"
#include "lora.h"
#include "packet.h"
#include "rx_history.h"





void setupBoard() {
  currentMethod = "setup()";
  systemState = INIT; 
  
  delay(1000);
  Serial.begin(115200);
  while (!Serial);

  // Load config BEFORE radio setup
  loadConfig();
 
  setupDisplay();
  delay(1000);

  setupLora();
  delay(1000);

  //setupWifi();
  //delay(1000);

  // setup done
  setStatus(READY, "loop()");
  delay(1000);
}




void setup() {
  setupBoard();
}


void loop() {
  updateStatus();
  handleSerialCommand();
  
  switch ( systemState ) {
    case INIT:
      // Serial.println("* INIT");
      setupBoard();
      break;
    
    case READY:
      // Serial.println("* READY");
      setStatus(IDLE, "loop");
      break;
      
    case DO_RX:
      // Serial.println("* DO_RX");
      decodePacket();
      break;
      
    case DO_TX:
      // Serial.println("* DO_TX");
      transmitPacket();
      break;
    
    case IN_RX:
      // should never fall into this state
      // Serial.println("* IN_RX");
      break;
      
    case IN_TX:
      // should never fall into this state
      // Serial.println("* IN_TX");
      break;
      
    case IDLE:
      delay(10);

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
      while (1);
  }
  
}
