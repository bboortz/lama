#ifndef NODES_H
#define NODES_H

#include <Arduino.h>

// Broadcast address
#define ADDR_BROADCAST 0xFF

// Node info for tracking
#define MAX_NODES 16

struct NodeInfo {
  uint8_t  nodeId;
  char     name[12];
  uint8_t  capabilities;
  uint32_t lastSeen;
  float    lastRssi;
  float    lastSnr;
  float    lastFreqErr;
  float    minRssi;
  float    minSnr;
  float    minFreqErr;
  float    maxRssi;
  float    maxSnr;
  float    maxFreqErr;
  float    avgRssi;
  float    avgSnr;
  float    avgFreqErr;
  uint16_t rxCount;
  uint16_t lostCount;
  int      lastSeq;
};

// Node registry
NodeInfo knownNodes[MAX_NODES];
int      nodeCount = 0;

// Function declarations

NodeInfo* findNode(uint8_t nodeId);
NodeInfo* addNode(uint8_t nodeId);
void      updateNodeStats(uint8_t nodeId, float rssi, float snr, int seq);
void      printNodes();

#endif
