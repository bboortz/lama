void setupDisplay() {
  // init display variable
  display = new Adafruit_SSD1306(config.screenWidth, config.screenHeight, &Wire, -1);

  // Init OLED
  Wire.begin(config.oledPinSda, config.oledPinScl);
  // Wire.begin(DEFAULT_OLED_SDA, DEFAULT_OLED_SCL);
  if (display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    display->setCursor(0, 0);
    setStatus(systemState, currentMethod);
  }
}


void updateStatus(void) {
  int totalExpected = rxPacketCount + rxPacketLost;
  int lossPercent = (totalExpected > 0) ? (100 * rxPacketLost / totalExpected) : 0;

  String header = getProgressStar() + " LAMA " + padRight(getProgressStar(), 14);
  String stateMessage = getState();
  String functionMessage = "F " + currentMethod;
  String statsMessage = "R:" + String(rxPacketCount) + 
                        " T:" + String(txPacketCount) + 
                        " L:" + String(lossPercent) + "%";

  //Serial.println(String(millis()) + " " + stateMessage + " " + statsMessage);

  display->clearDisplay();
  display->setCursor(0, 0);
  display->println(header);
  display->println(stateMessage);
  display->println(statsMessage);
  // display->println(functionMessage);
  display->println(txMessage);

  // Per-user signal quality
  display->println("      User SEQ SNR L%");
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
    display->println(line);
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

  display->display();
}


void displayError(String msg) {
  display->setCursor(0, 0);
  display->println(msg);
  display->display();
}

