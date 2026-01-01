

// Generic right-pad function
String padRight(String str, int width) {
  while (str.length() < width) {
    str = " " + str;
  }
  return str;
}

String padZeroRight(int num, int digits) {
  char buffer[8];
  sprintf(buffer, "%0*d", digits, num % (int)pow(10, digits));
  return String(buffer);
}

String getProgressStar() {
  static int   counter = 0;
  const String chars[] = {"/", "-", "\\", "|"};

  String result = chars[counter % 4];
  counter++;

  return result;
}

uint16_t readBattery() {
  // T3: ADC on GPIO35 with voltage divider
  // Adjust formula based on your hardware
  int raw = analogRead(35);
  return (raw * 2 * 3300) / 4095;  // Returns mV
}