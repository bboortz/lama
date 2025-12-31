/*
TODOS




*/

#include <SPI.h>
#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "packet.h"

// LILYGO T3 V1.6.x pin definitions
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 23
#define LORA_IRQ 26

// OLED
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64


// Create SX1276 instance
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// State
enum system_state_enum {INIT, READY, DO_RX, DO_TX, IN_RX, IN_TX, IDLE, FAILED};
system_state_enum systemState = INIT;
int radiolibState = 0;
int rxState = 0;
int txState = 0;
enum network_state_enum {LOST, CONNECTED};
network_state_enum networkState = LOST;
String currentMethod = "setup";

// RX TX intervals
unsigned long lastTxTime = 0;
unsigned long lastRxTime = 0; 

// last tx message
String txMessage = "-";

// Message history structure
int rxPacketLost = 0;
int rxPacketCount = 0;
int txPacketCount = 0;
struct RxMessage {
  String user;
  int seq;        // message id
  int msecs;      // milliseconds
  float snr;      // SNR we measured when receiving
  float rssi; 
  float freqErr;
  float rsnr;     // SNR they reported (from our TX)
  
};
#define RX_HISTORY_SIZE 3
RxMessage rxHistory[RX_HISTORY_SIZE];
int rxHistoryCount = 0;

// user stats
#define MAX_USERS 5
struct UserStats {
  String name;
  int lastSeq;
  int received;
  int lost;
  // Signal quality per user
  float lastRssi;
  float lastSnr;
  float lastFreqErr;
  float minRssi;
  float maxRssi;
  float avgRssi;
  float avgSnr;
  float avgFreqErr;
};
UserStats userStats[MAX_USERS];
int userCount = 0;

// Config structure
struct Config {
  uint8_t nodeId;           
  uint8_t networkId;        
  uint8_t capabilities;
  char user[16];
  float loraFrequency;
  float loraBw;
  int loraSf;
  int loraCr;
  int loraSync;
  int loraPreamble;
  int loraTxPower;
  bool loraCrc;
  bool loraAfc;
  float loraAfcBandwidth;
  int txInterval;
  int rxTimeout;
};

Config config;


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



void setup_board() {
  currentMethod = "setup_board()";
  systemState = INIT; 
  
  delay(1000);
  Serial.begin(115200);
  while (!Serial);

  // Load config BEFORE radio setup
  loadConfig();
 
  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    setStatus(systemState, currentMethod);
  }

  delay(1000);

  // Init SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // setup lora
  setStatus(systemState, currentMethod);
  radiolibState = radio.begin(
    config.loraFrequency,
    config.loraBw,
    config.loraSf,
    config.loraCr,
    config.loraSync,
    config.loraTxPower,             // output power (not used for RX)
    config.loraPreamble
  );
  if ( doRadiolibState(radiolibState) ) {
    return;
  }
  // Enable AFC (Automatic Frequency Correction)
  radio.setAFCBandwidth(config.loraAfcBandwidth);
  radio.setAFC(config.loraAfc);
  // Enable CRC
  radio.setCRC(config.loraCrc); 
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
        lastTxTime = millis();
      }
      
      break;
 
    case FAILED:
      Serial.printf("Failed! Error code: %d\n", systemState);
      display.println("INIT FAILED!");
      display.display();
      while (1);
  }
  
}
