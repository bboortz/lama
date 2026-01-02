
#ifndef LORA_STATS_H
#define LORA_STATS_H

// Message history structure
uint16_t loraRxPacketLost  = 0;
uint16_t loraRxPacketCount = 0;
uint16_t loraTxPacketCount = 0;

struct LoraRxStatsMessage {
  uint8_t  nodeId;
  uint16_t seq;    // message id
  uint16_t msecs;  // milliseconds
  float    snr;    // SNR we measured when receiving
  float    rssi;
  float    freqErr;
  float    rsnr;  // SNR they reported (from our TX)
};
#define LORA_RX_STATS_MESSAGE_SIZE 30
LoraRxStatsMessage loraRxHistory[LORA_RX_STATS_MESSAGE_SIZE];
int                loraRxHistoryCount = 0;

#endif