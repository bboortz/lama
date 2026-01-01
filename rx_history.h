
#ifndef RX_HISTORY_H
#define RX_HISTORY_H

// Message history structure
uint16_t rxPacketLost  = 0;
uint16_t rxPacketCount = 0;
uint16_t txPacketCount = 0;
struct RxMessage {
  String user;
  uint16_t    seq;    // message id
  uint16_t    msecs;  // milliseconds
  float  snr;    // SNR we measured when receiving
  float  rssi;
  float  freqErr;
  float  rsnr;  // SNR they reported (from our TX)
};
#define RX_HISTORY_SIZE 3
RxMessage rxHistory[RX_HISTORY_SIZE];
int       rxHistoryCount = 0;

// user stats
#define MAX_USERS 5
struct UserStats {
  String name;
  int32_t    lastSeq;
  int32_t    received;
  int32_t    lost;
  // Signal quality per user
  float lastRssi;
  float lastSnr;
  float lastFreqErr;
  float minRssi;
  float maxRssi;
  float avgRssi;
  float avgSnr;
  float avgFreqErr;
};
UserStats userStats[MAX_USERS];
uint8_t       userCount = 0;

#endif