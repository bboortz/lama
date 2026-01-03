
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ERROR_HISTORY_SIZE 3
String errorHistory[ERROR_HISTORY_SIZE];
int    errorHistoryCount = 0;

// last tx message
String txMessage = "           ";

bool displayAvailable = false;
// Adafruit_SSD1306 display(config.screenWidth, config.screenHeight, &Wire, -1);
// Adafruit_SSD1306 display(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, &Wire, -1);
// Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_SSD1306* display = nullptr;

#endif