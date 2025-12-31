void enableRX(void) {
  radiolibState = radio.startReceive();
  if ( doRadiolibState(radiolibState) ) {
    return;
  }
}


// ISR - keep short!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void onPacketRX(void) {
  setSystemState(DO_RX);
}


void parsePacket(String data, String *user, int *seq, int *msecs, float *rsnr) {
    char buffer[64];
    data.toCharArray(buffer, sizeof(buffer));
    char userBuf[32];
    
    int result = sscanf(buffer, "%s %d %d %f", userBuf, seq, msecs, rsnr);

    if (result == 4) {
        // Format: user seq milliseconds rsnr
        *user = String(userBuf);
    } else if (result == 3) {  
        // Format: user seq milliseconds
        *user = String(userBuf);
        *rsnr = 0.0;
    } else if (result == 2) {
        // Format: user seq
        *user = String(userBuf);
        *msecs = 0;
        *rsnr = 0.0;
    } else {
        // Parse error
        *user = "";
        *seq = 0;
        *msecs = 0;
        *rsnr = 0.0;
    }
    
}


void decodePacket(void) {
  if (systemState != DO_RX) {
    return;
  }
  setStatus(IN_RX, "decodePacket()");

  String data;
  rxState = radio.readData(data);
  if (radio.getPacketLength() != 0) {
    if (rxState == RADIOLIB_ERR_NONE) {
      float freqErr = radio.getFrequencyError();
      float rssi = radio.getRSSI();
      float snr = radio.getSNR();
      
      // parse packet
      String receivedUsername;
      int receivedSeq;
      int receivedMSecs;
      float receivedSnr;
      parsePacket(data, &receivedUsername, &receivedSeq, &receivedMSecs, &receivedSnr);

      // loss detection
      UserStats* stats = findOrCreateUser(receivedUsername);
      if (stats) {
        if (stats->lastSeq >= 0 && receivedSeq > stats->lastSeq + 1) {
          int lost = receivedSeq - stats->lastSeq - 1;
          stats->lost += lost;
          rxPacketLost += lost;  
          Serial.printf("*** LOST %d packets!\n", lost);
        }
        stats->lastSeq = receivedSeq;
        stats->received++;
      }

      // Update signal stats for this user
      updateUserSignalStats(stats, rssi, snr, freqErr);
        
      // Warn on degrading signal
      if (stats->received > 10 && rssi < stats->avgRssi - 10) {
        Serial.printf("WARNING: %s signal degraded by %.0f dB!\n", 
                      stats->name.c_str(), stats->avgRssi - rssi);
      }

      // add rx to history
      rxPacketCount++;
      lastRxTime = millis();
      setNetworkState(CONNECTED);
      addToRxHistory(receivedUsername, receivedSeq, receivedMSecs, snr, rssi, freqErr, receivedSnr); 
     
    } else if (rxState == RADIOLIB_ERR_RX_TIMEOUT) {
      // Timeout - no packet, just keep listening
      
    } else if (rxState == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("CRC error!");
      
    } else {
      Serial.printf("Receive failed, error: %d\n", rxState);

    }
    
    updateStatus();
      
  }

  setStatus(IDLE, "loop()");
}


void transmitPacket(void) {
  if (systemState != DO_TX) {
    return;
  }
  setStatus(IN_TX, "transmitPacket()");
  txPacketCount++;

  // Build message
  int m = millis() % 100;
  txMessage = String(config.user) + " " + padZeroRight(txPacketCount, 3) + " " + padZeroRight(m, 3) + " " + String(getLastSnr()); 

  txState = radio.transmit(txMessage);
  enableRX();
  updateStatus();

  setStatus(IDLE, "loop()");
}
