
#ifndef STATE_H
#define STATE_H

// State
enum system_state_enum {INIT, READY, DO_RX, DO_TX, IN_RX, IN_TX, IDLE, FAILED};
system_state_enum systemState = INIT;
int radiolibState = 0;
int rxState = 0;
int txState = 0;
enum network_state_enum {LOST, CONNECTED};
network_state_enum networkState = LOST;
String currentMethod = "setup";

#endif