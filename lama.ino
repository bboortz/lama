
#include <SPI.h>

#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "state.h"
#include "display.h"
#include "lora.h"
#include "packet.h"
#include "rx_history.h"



void updateStatus(void) {
  int totalExpected = rxPacketCount + rxPacketLost;
  int lossPercent = (totalExpected > 0) ? (100 * rxPacketLost / totalExpected) : 0;

  String header = getProgressStar() + " LoraTopfschlagen " + getProgressStar();
  String stateMessage = getState();
  String functionMessage = "F " + currentMethod;
  String statsMessage = "R:" + String(rxPacketCount) + 
                        " T:" + String(txPacketCount) + 
                        " L:" + String(lossPercent) + "%";

  //Serial.println(String(millis()) + " " + stateMessage + " " + statsMessage);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(header);
  display.println(stateMessage);
  display.println(statsMessage);
  // display.println(functionMessage);
  display.println(txMessage);

  // Per-user signal quality
  display.println("      User SEQ SNR L%");
  for (int i = 0; i < userCount && i < 3; i++) {
    UserStats* s = &userStats[i];
    int total = s->received + s->lost;
    int lossPct = (total > 0) ? (100 * s->lost / total) : 0;
    
    char line[22];
    sprintf(line, "%10s %03d %3.0f %2d",
            s->name.substring(0, 6).c_str(),
            s->lastSeq % 1000,
            // s->avgRssi,
            s->avgSnr,
            lossPct);
    display.println(line);
  }

/*
  // Display header for history
  display.println("   User  ID SNR RSNR");
  
  // Display message history
  for (int i = 0; i < rxHistoryCount; i++) {
    char line[22];
    sprintf(line, "%7s %03d %3.0f %4.0f",
            rxHistory[i].user.substring(0, 6).c_str(),
            rxHistory[i].seq % 1000,
            rxHistory[i].snr,
            rxHistory[i].rsnr);
    display.println(line);
  }
  */

  display.display();
}



void setup_wifi() {


}



void setup_board() {
  currentMethod = "setup_board()";
  systemState = INIT; 
  
  delay(1000);
  Serial.begin(115200);
  while (!Serial);

  // Load config BEFORE radio setup
  loadConfig();
 
  setup_display();
  delay(1000);

  setup_lora();
  delay(1000);

  // setup receiver
  setStatus(systemState, currentMethod);
  radio.setPacketReceivedAction(onPacketRX);
  enableRX();
  delay(1000);

  // setup done
  setStatus(READY, "loop()");
  delay(1000);
}


void setup() {
  setup_board();
}


void loop() {
  updateStatus();
  handleSerialCommand();
  
  switch ( systemState ) {
    case INIT:
      // Serial.println("* INIT");
      setup_board();
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
        setNetworkState(LOST);
      }

      if (millis() - lastTxTime > config.txInterval) {
        setSystemState(DO_TX);
        
      }
      
      break;
 
    case FAILED:
      Serial.printf("Failed! Error code: %d\n", systemState);
      display.println("INIT FAILED!");
      display.display();
      while (1);
  }
  
}
