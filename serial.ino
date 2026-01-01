
void setupSerial() {
  // serial setup
  Serial.begin(115200);
#if SERIAL_WAIT_TIMEOUT > 0
  unsigned long serialTimeout = millis() + SERIAL_WAIT_TIMEOUT;
  while (!Serial && millis() < serialTimeout) {
    delay(10);
  }
#elif SERIAL_WAIT_TIMEOUT < 0
  while (!Serial)
    ;  // Wait forever (debugging only)
#endif
}