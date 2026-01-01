

void addToRxHistory(String user,
                    int    seq,
                    int    msecs,
                    float  snr,
                    float  rssi,
                    float  freqErr,
                    float  rsnr) {
  // Shift older messages down
  for (int i = RX_HISTORY_SIZE - 1; i > 0; i--) {
    rxHistory[i] = rxHistory[i - 1];
  }

  // Add new message at top
  rxHistory[0].user    = user;
  rxHistory[0].seq     = seq;
  rxHistory[0].msecs   = msecs;
  rxHistory[0].snr     = snr;
  rxHistory[0].rssi    = rssi;
  rxHistory[0].freqErr = freqErr;
  rxHistory[0].rsnr    = rsnr;

  if (rxHistoryCount < RX_HISTORY_SIZE) {
    rxHistoryCount++;
  }
}

UserStats* findOrCreateUser(String name) {
  // Find existing
  for (int i = 0; i < userCount; i++) {
    if (userStats[i].name == name) {
      return &userStats[i];
    }
  }
  // Create new
  if (userCount < MAX_USERS) {
    userStats[userCount].name        = name;
    userStats[userCount].lastSeq     = 0;
    userStats[userCount].received    = 0;
    userStats[userCount].lost        = 0;
    userStats[userCount].lastRssi    = 0;
    userStats[userCount].lastSnr     = 0;
    userStats[userCount].lastFreqErr = 0;
    userStats[userCount].minRssi     = 0;
    userStats[userCount].maxRssi     = -150;
    userStats[userCount].avgRssi     = 0;
    userStats[userCount].avgSnr      = 0;
    userStats[userCount].avgFreqErr  = 0;
    return &userStats[userCount++];
  }
  return nullptr;
}

void updateUserSignalStats(UserStats* stats, float rssi, float snr, float freqErr) {
  stats->lastRssi    = rssi;
  stats->lastSnr     = snr;
  stats->lastFreqErr = freqErr;

  // Min/Max
  if (rssi < stats->minRssi || stats->received == 1) {
    stats->minRssi = rssi;
  }
  if (rssi > stats->maxRssi) {
    stats->maxRssi = rssi;
  }

  // Running average (exponential moving average)

  if (stats->received == 1) {
    stats->avgRssi    = rssi;
    stats->avgSnr     = snr;
    stats->avgFreqErr = freqErr;
  } else {
    float alpha       = 0.2;
    stats->avgRssi    = alpha * rssi + (1 - alpha) * stats->avgRssi;
    stats->avgSnr     = alpha * snr + (1 - alpha) * stats->avgSnr;
    stats->avgFreqErr = alpha * freqErr + (1 - alpha) * stats->avgFreqErr;
  }
}

void printUserStats() {
  Serial.println("=== User Statistics ===");
  for (int i = 0; i < userCount; i++) {
    UserStats* s       = &userStats[i];
    int        total   = s->received + s->lost;
    int        lossPct = (total > 0) ? (100 * s->lost / total) : 0;

    Serial.printf("\n[%s]\n", s->name.c_str());
    Serial.printf("  Packets:  %d RX, %d lost (%d%%)\n", s->received, s->lost, lossPct);
    Serial.printf("  RSSI:     %.0f dBm (avg: %.0f, min: %.0f, max: %.0f)\n",
                  s->lastRssi,
                  s->avgRssi,
                  s->minRssi,
                  s->maxRssi);
    Serial.printf("  SNR:      %.1f dB (avg: %.1f)\n", s->lastSnr, s->avgSnr);
    Serial.printf("  FreqErr:  %.0f Hz (avg: %.0f)\n", s->lastFreqErr, s->avgFreqErr);

    // Link quality assessment
    String quality;
    if (s->avgRssi > -85 && lossPct < 5) {
      quality = "Excellent";
    } else if (s->avgRssi > -100 && lossPct < 10) {
      quality = "Good";
    } else if (s->avgRssi > -110 && lossPct < 20) {
      quality = "Fair";
    } else {
      quality = "Poor";
    }
    Serial.printf("  Quality:  %s\n", quality.c_str());
  }
  Serial.println("\n=======================");
}

float getLastSnr() {
  if (rxHistoryCount > 0) {
    return rxHistory[0].snr;
  }
  return 0.0;
}

float getLastRsnr() {
  if (rxHistoryCount > 0) {
    return rxHistory[0].rsnr;
  }
  return 0.0;
}