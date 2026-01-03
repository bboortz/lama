#include "arduino_bridge.h"

#include <Arduino.h>

// Make it callable from C
extern "C" void arduino_setup(void) {
  setup();
}

// Make it callable from C
extern "C" void arduino_loop(void) {
  loop();
}
