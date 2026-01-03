

void addToLoraRxStats(uint8_t nodeId,
                      int     seq,
                      int     msecs,
                      float   snr,
                      float   rssi,
                      float   freqErr,
                      float   rsnr) {
  // Shift older messages down
  for (int i = LORA_RX_STATS_MESSAGE_SIZE - 1; i > 0; i--) {
    loraRxHistory[i] = loraRxHistory[i - 1];
  }

  // Add new message at top
  loraRxHistory[0].nodeId  = nodeId;
  loraRxHistory[0].seq     = seq;
  loraRxHistory[0].msecs   = msecs;
  loraRxHistory[0].snr     = snr;
  loraRxHistory[0].rssi    = rssi;
  loraRxHistory[0].freqErr = freqErr;
  loraRxHistory[0].rsnr    = rsnr;

  if (loraRxHistoryCount < LORA_RX_STATS_MESSAGE_SIZE) {
    loraRxHistoryCount++;
  }
}

void printLoraRxStats() {
  Serial.println("=== Lora RX Statistics ===");

  for (int i = 0; i < loraRxHistoryCount; i++) {
    LoraRxStatsMessage* m = &loraRxHistory[i];

    Serial.printf("[%5d] seq=%-5d snr=%4.1f rssi=%-4.1f ferr=%-4.1f msecs=%-5d rsnr=%4.1f\n",
                  m->nodeId,
                  m->seq,
                  m->snr,
                  m->rssi,
                  m->freqErr,
                  m->msecs,
                  m->rsnr);
  }
}

float getLastSnr() {
  if (loraRxHistoryCount > 0) {
    return loraRxHistory[0].snr;
  }
  return 0.0;
}

float getLastRsnr() {
  if (loraRxHistoryCount > 0) {
    return loraRxHistory[0].rsnr;
  }
  return 0.0;
}