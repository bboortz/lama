
#ifndef RX_HISTORY_H
#define RX_HISTORY_H

// last tx message
String txMessage = "-";

// Message history structure
int rxPacketLost  = 0;
int rxPacketCount = 0;
int txPacketCount = 0;
struct RxMessage {
  String user;
  int    seq;    // message id
  int    msecs;  // milliseconds
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
  int    lastSeq;
  int    received;
  int    lost;
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
int       userCount = 0;

#endif