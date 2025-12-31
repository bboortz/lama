


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
  static int counter = 0;
  String chars[] = {"/", "-", "\\", "|"};
  
  String result = chars[counter % 4];
  counter++;
  
  return result;
}

String snrToStars(float snr) {
  int stars;
  
  // Determine number of stars based on SNR
  if (snr > 15) {
    stars = 11;
  } else if (snr > 12) {
    stars = 10;
  } else if (snr > 9) {
    stars = 9;
  } else if (snr > 6) {
    stars = 8;
  } else if (snr > 3) {
    stars = 7;
  } else if (snr > 0) {
    stars = 6;
  } else if (snr > -3) {
    stars = 5;
  } else if (snr > -6) {
    stars = 4;
  } else if (snr > -9) {
    stars = 3;
  } else if (snr > -12) {
    stars = 2;
  } else {
    stars = 1;
  }
  
  // Build the string with stars and padding
  String result = "";
  
  // Add leading spaces
  for (int i = 0; i < (11 - stars); i++) {
    result += " ";
  }
  
  // Add stars (using | character)
  for (int i = 0; i < stars; i++) {
    result += "*";
  }
  
  return result;
}

uint16_t readBattery() {
  // T3: ADC on GPIO35 with voltage divider
  // Adjust formula based on your hardware
  int raw = analogRead(35);
  return (raw * 2 * 3300) / 4095;  // Returns mV
}