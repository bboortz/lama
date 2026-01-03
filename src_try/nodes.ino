#include "nodes.h"

// ============== Node Registry ==============

NodeInfo* findNode(uint8_t nodeId) {
  for (int i = 0; i < nodeCount; i++) {
    if (knownNodes[i].nodeId == nodeId) {
      return &knownNodes[i];
    }
  }
  return nullptr;
}

NodeInfo* addNode(uint8_t nodeId) {
  if (nodeCount >= MAX_NODES) {
    Serial.println("Node registry full!");
    return nullptr;
  }

  NodeInfo* node = &knownNodes[nodeCount++];
  memset(node, 0, sizeof(NodeInfo));
  node->nodeId  = nodeId;
  node->lastSeq = -1;
  sprintf(node->name, "Node%d", nodeId);

  Serial.printf("Added new node: %d\n", nodeId);
  return node;
}

void updateNodeStats(uint8_t nodeId, float rssi, float snr, float freqErr, int seq) {
  NodeInfo* node = findNode(nodeId);
  if (!node) {
    node = addNode(nodeId);
    if (!node)
      return;
  }

  // Packet loss detection
  if (node->lastSeq >= 0) {
    int expected = (node->lastSeq + 1) & 0xFFFF;
    if (seq != expected && seq > node->lastSeq) {
      int lost = seq - expected;
      if (lost > 0 && lost < 100) {  // Sanity check
        node->lostCount += lost;
        Serial.printf("Node %d: lost %d packets\n", nodeId, lost);
      }
    }
  }

  // Min/Max
  if (rssi < node->minRssi || node->lastSeen == -1) {
    node->minRssi = rssi;
  }
  if (rssi > node->maxRssi || node->lastSeen == -1) {
    node->maxRssi = rssi;
  }
  if (snr < node->minSnr || node->lastSeen == -1) {
    node->minSnr = snr;
  }
  if (snr > node->maxSnr || node->lastSeen == -1) {
    node->maxSnr = snr;
  }
  if (freqErr < node->minFreqErr || node->lastSeen == -1) {
    node->minFreqErr = freqErr;
  }
  if (freqErr > node->maxFreqErr || node->lastSeen == -1) {
    node->maxFreqErr = freqErr;
  }

  // Average calculation

  if (node->lastSeen == -1) {
    node->avgRssi    = rssi;
    node->avgSnr     = snr;
    node->avgFreqErr = freqErr;
  } else {
    float alpha      = 0.2;
    node->avgRssi    = alpha * rssi + (1 - alpha) * node->avgRssi;
    node->avgSnr     = alpha * snr + (1 - alpha) * node->avgSnr;
    node->avgFreqErr = alpha * freqErr + (1 - alpha) * node->avgFreqErr;
  }

  // last
  node->lastSeq     = seq;
  node->lastSeen    = millis();
  node->lastRssi    = rssi;
  node->lastSnr     = snr;
  node->lastFreqErr = freqErr;
  node->rxCount++;
}

void printNodes() {
  Serial.println("\n=== Known Nodes ===");
  for (int i = 0; i < nodeCount; i++) {
    NodeInfo* n       = &knownNodes[i];
    int       total   = n->rxCount + n->lostCount;
    int       lossPct = (total > 0) ? (100 * n->lostCount / total) : 0;

    Serial.printf(
        "[%5d] %8s: rx=%-5d seq=%-5d asnr=%-4.1f arssi=%-4.1f aferr=%-4.1f lost=%-4d (%d%%) "
        "snr=%4.1f rssi=%-4.1f ferr=%-4.1f last=%ds ago\n",
        n->nodeId,
        n->name,
        n->rxCount,
        n->lastSeq,
        n->avgSnr,
        n->avgRssi,
        n->avgFreqErr,
        n->lostCount,
        lossPct,
        n->lastSnr,
        n->lastRssi,
        n->lastFreqErr,
        (millis() - n->lastSeen) / 1000);
  }
}
