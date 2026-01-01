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

void addError(String msg) {
  // Shift older messages down
  for (int i = ERROR_HISTORY_SIZE - 1; i > 0; i--) {
    errorHistory[i] = errorHistory[i - 1];
  }

  // Add new message at top
  errorHistory[0] = msg;

  if (errorHistoryCount < ERROR_HISTORY_SIZE) {
    errorHistoryCount++;
  }

  setSystemState(ERROR);
}

void displayStatusHeader(void) {
  if (!displayAvailable) {
    return;  // check if a display is available
  }

  uint32_t           totalExpected = rxPacketCount + rxPacketLost;
  uint32_t           lossPercent   = (totalExpected > 0) ? (100 * rxPacketLost / totalExpected) : 0;
  uint32_t rxAgo         = (lastRxTime > 0) ? (millis() - lastRxTime) / 1000 : 999;

  String header       = getProgressStar() + " LAMA " + txMessage + padRight(getProgressStar(), 3);
  String stateMessage = getState();
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
}

void displayStatus(void) {
  if (!displayAvailable) {
    return;  // check if a display is available
  }

  displayStatusHeader();

  if (systemState != FATAL) {
    display->println(errorHistory[0]);

    // Per-user signal quality

    if (userCount > 0) {
      display->println(" Node SEQ ASNR SNR L%");
      for (int i = 0; i < userCount && i < 3; i++) {
        UserStats* s       = &userStats[i];
        int        total   = s->received + s->lost;
        int        lossPct = (total > 0) ? (100 * s->lost / total) : 0;

        char line[22];
        sprintf(line,
                "%5s %03d %4.0f %3.0f %2d",
                s->name.substring(0, 6).c_str(),
                s->lastSeq % 1000,
                s->avgSnr,
                s->lastSnr,
                lossPct);
        display->println(line);
      }
    }
  } else {
    display->println(currentMethod);
    display->println("RL:" + String(radiolibState) + " RX:" + String(rxState)
                     + " TX:" + String(txState));
    displayAllErrors();
  }

  display->display();
}

void displayAllErrors(void) {
  if (!displayAvailable) {
    return;  // check if a display is available
  }

  for (int i = 0; i < ERROR_HISTORY_SIZE; i++) {
    display->println(errorHistory[i]);
  }

  display->display();
}
