void setupDisplay() {
  // init display variable
  display = new Adafruit_SSD1306(config.screenWidth, config.screenHeight, &Wire, -1);

  // Init OLED
  Wire.begin(config.oledPinSda, config.oledPinScl);
  // Wire.begin(DEFAULT_OLED_SDA, DEFAULT_OLED_SCL);
  if (display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayAvailable = true;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    display->setCursor(0, 0);
    setStatus(systemState);
  } else {
    displayAvailable = false;  // Mark as unavailable
    Serial.println("WARNING: Display initialization failed!");
  }
}

void updateStatus(void) {
  if (!displayAvailable) {
    return;  // check if a display is available
  }

  int           totalExpected = rxPacketCount + rxPacketLost;
  int           lossPercent   = (totalExpected > 0) ? (100 * rxPacketLost / totalExpected) : 0;
  unsigned long rxAgo         = (lastRxTime > 0) ? (millis() - lastRxTime) / 1000 : 999;

  String header          = getProgressStar() + " LAMA " + padRight(getProgressStar(), 14);
  String stateMessage    = getState();
  String functionMessage = "F " + currentMethod;
  String statsMessage    = padRight("R:" + String(rxPacketCount), 5) + " "
                        + padRight("T:" + String(txPacketCount), 5) + " "
                        + padRight("L:" + String(lossPercent) + "%", 5) + " "
                        + padRight(String(rxAgo), 3);

  display->clearDisplay();
  display->setCursor(0, 0);
  display->println(header);
  display->println(stateMessage);
  display->println(statsMessage);
  display->println(txMessage);

  // Per-user signal quality
  display->println("      User SEQ SNR L%");
  for (int i = 0; i < userCount && i < 3; i++) {
    UserStats* s       = &userStats[i];
    int        total   = s->received + s->lost;
    int        lossPct = (total > 0) ? (100 * s->lost / total) : 0;

    char line[22];
    sprintf(line,
            "%10s %03d %3.0f %2d",
            s->name.substring(0, 6).c_str(),
            s->lastSeq % 1000,
            // s->avgRssi,
            s->avgSnr,
            lossPct);
    display->println(line);
  }

  display->display();
}

void displayError(String msg) {
  setSystemState(FAILED);

  if (!displayAvailable) {
    Serial.printf("ERROR (no display): %s\n", msg.c_str());
    return;
  }

  display->setCursor(0, 0);
  display->println(msg);
  display->display();
}
