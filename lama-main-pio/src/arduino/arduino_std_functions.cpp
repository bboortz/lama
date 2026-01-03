#include <Arduino.h>

#include "config.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Arduino setup called");

  loadConfig();
  saveConfig();
  printConfig();
}

void loop() {
  // Just a test
  Serial.println("Arduino loop tick");
  delay(1000);
}
