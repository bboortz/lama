
ParsedLoraPacket parseLoraPacket(String data) {
  ParsedLoraPacket result = {"", 0, 0, 0.0};

  char buffer[64];
  data.toCharArray(buffer, sizeof(buffer));

  char userBuf[32];
  int  scanResult =
      sscanf(buffer, "%31s %d %d %f", userBuf, &result.seq, &result.msecs, &result.rsnr);

  if (strlen(userBuf) > 0) {
    result.user = String(userBuf);
  }

  return result;
}

void decodeLoraPacket(void) {
  String data;
  rxState = radio->readData(data);

  if (radio->getPacketLength() != 0) {
    if (rxState == RADIOLIB_ERR_NONE) {
      float freqErr = radio->getFrequencyError();
      float rssi    = radio->getRSSI();
      float snr     = radio->getSNR();

      // Parse packet - much cleaner!
      ParsedLoraPacket packet = parseLoraPacket(data);

      // Loss detection
      UserStats* stats = findOrCreateUser(packet.user);
      if (stats) {
        if (stats->lastSeq >= 0 && packet.seq > stats->lastSeq + 1) {
          int lost = packet.seq - stats->lastSeq - 1;
          stats->lost += lost;
          rxPacketLost += lost;
          Serial.printf("*** LOST %d packets!\n", lost);
        }
        stats->lastSeq = packet.seq;
        stats->received++;

        updateUserSignalStats(stats, rssi, snr, freqErr);

        if (stats->received > 10 && rssi < stats->avgRssi - 10) {
          Serial.printf("WARNING: %s signal degraded by %.0f dB!\n",
                        stats->name.c_str(),
                        stats->avgRssi - rssi);
        }
      }

      rxPacketCount++;
      lastRxTime = millis();
      setLoraNetworkState(CONNECTED);
      addToRxHistory(packet.user, packet.seq, packet.msecs, snr, rssi, freqErr, packet.rsnr);

    } else {
      handleRxError(rxState);
    }
  }
}

ParsedLoraPacket createLoraPacket(String user, int seq, int msecs, float rsnr) {
  ParsedLoraPacket result = {"", 0, 0, 0.0};
  result.user             = user;
  result.seq              = seq;
  result.msecs            = msecs;
  result.rsnr             = rsnr;

  return result;
}
