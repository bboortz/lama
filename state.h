
#ifndef STATE_H
#define STATE_H

#include <cstdint>

// State
enum SystemStateEnum : uint8_t {
  INIT   = 0x00,
  READY  = 0x01,
  DO_RX  = 0x02,
  DO_TX  = 0x03,
  IN_RX  = 0x04,
  IN_TX  = 0x05,
  IDLE   = 0x06,
  FAILED = 0x07
};
SystemStateEnum systemState = INIT;

enum NetworkStateEnum : uint8_t { LOST = 0x00, CONNECTED = 0x01 };
NetworkStateEnum loraNetworkState = LOST;
NetworkStateEnum wifiNetworkState = LOST;

int radiolibState = 0;
int rxState       = 0;
int txState       = 0;

String currentMethod = "setup";

#endif