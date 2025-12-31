#include "packet.h"

// Global packet state
uint16_t packetSeq = 0;
float lastRssi = 0;
float lastSnr = 0;

// Node registry
NodeInfo knownNodes[MAX_NODES];
int nodeCount = 0;

// ============== Packet Creation ==============

LoraPacket createPacket(PacketType type, uint8_t dst, uint8_t flags) {
  LoraPacket pkt = {0};
  pkt.header.version = 1;
  pkt.header.type = type;
  pkt.header.networkId = config.networkId;
  pkt.header.srcNode = config.nodeId;
  pkt.header.dstNode = dst;
  pkt.header.seq = ++packetSeq;
  pkt.header.flags = flags;
  pkt.header.dataLen = 0;
  return pkt;
}

int transmitPacket(LoraPacket* pkt) {
  int len = sizeof(PacketHeader) + pkt->header.dataLen;
  int state = radio->transmit((uint8_t*)pkt, len);
  
  Serial.printf("TX: type=%d dst=%d seq=%d len=%d\n",
                pkt->header.type, pkt->header.dstNode,
                pkt->header.seq, len);
  
  return state;
}

// ============== Send Functions ==============

void sendBeat() {
  LoraPacket pkt = createPacket(PKT_BEAT, ADDR_BROADCAST);
  
  BeatPayload* p = (BeatPayload*)pkt.payload;
  p->battery = readBattery();
  p->rssi = (int8_t)lastRssi;
  p->uptime = (millis() / 3600000) & 0xFF;
  
  pkt.header.dataLen = sizeof(BeatPayload);
  transmitPacket(&pkt);
}

void sendHealth() {
  LoraPacket pkt = createPacket(PKT_HEALTH, ADDR_BROADCAST);
  
  HealthPayload* p = (HealthPayload*)pkt.payload;
  p->battery = readBattery();
  p->rxCount = rxPacketCount & 0xFFFF;
  p->txCount = txPacketCount & 0xFFFF;
  p->lostCount = rxPacketLost & 0xFF;
  p->temperature = 0;  // TODO: read internal temp
  
  pkt.header.dataLen = sizeof(HealthPayload);
  transmitPacket(&pkt);
}

void sendPosition() {
  #ifdef HAS_GPS
  LoraPacket pkt = createPacket(PKT_POSITION, ADDR_BROADCAST);
  
  PositionPayload* p = (PositionPayload*)pkt.payload;
  p->latitude = gps.location.lat() * 1e7;
  p->longitude = gps.location.lng() * 1e7;
  p->satellites = gps.satellites.value();
  p->speed = min((int)gps.speed.kmph(), 255);
  p->heading = gps.course.deg() / 2;
  p->altitude = min((int)(gps.altitude.meters() / 10), 255);
  
  pkt.header.dataLen = sizeof(PositionPayload);
  transmitPacket(&pkt);
  #endif
}

void sendAlert(uint8_t alertType, uint8_t severity) {
  LoraPacket pkt = createPacket(PKT_ALERT, ADDR_BROADCAST, FLAG_ACK_REQ);
  
  AlertPayload* p = (AlertPayload*)pkt.payload;
  p->alertType = alertType;
  p->severity = severity;
  p->battery = readBattery();
  
  #ifdef HAS_GPS
  if (gps.location.isValid()) {
    p->latitude = gps.location.lat() * 1e7;
    p->longitude = gps.location.lng() * 1e7;
  } else {
    p->latitude = 0;
    p->longitude = 0;
  }
  #else
  p->latitude = 0;
  p->longitude = 0;
  #endif
  
  pkt.header.dataLen = sizeof(AlertPayload);
  transmitPacket(&pkt);
}

void sendPing(uint8_t dstNode) {
  LoraPacket pkt = createPacket(PKT_PING, dstNode);
  
  PingPayload* p = (PingPayload*)pkt.payload;
  p->timestamp = millis();
  
  pkt.header.dataLen = sizeof(PingPayload);
  transmitPacket(&pkt);
}

void sendPong(uint8_t dstNode, uint32_t timestamp) {
  LoraPacket pkt = createPacket(PKT_PONG, dstNode);
  
  PingPayload* p = (PingPayload*)pkt.payload;
  p->timestamp = timestamp;
  
  pkt.header.dataLen = sizeof(PingPayload);
  transmitPacket(&pkt);
}

void sendAck(uint8_t dstNode, uint16_t ackedSeq) {
  LoraPacket pkt = createPacket(PKT_ACK, dstNode);
  
  AckPayload* p = (AckPayload*)pkt.payload;
  p->ackedSeq = ackedSeq;
  
  pkt.header.dataLen = sizeof(AckPayload);
  transmitPacket(&pkt);
}

void sendAnnounce() {
  LoraPacket pkt = createPacket(PKT_ANNOUNCE, ADDR_BROADCAST);
  
  AnnouncePayload* p = (AnnouncePayload*)pkt.payload;
  strncpy(p->name, config.user, 12);
  p->capabilities = config.capabilities;
  p->version = 1;
  p->battery = readBattery();
  
  pkt.header.dataLen = sizeof(AnnouncePayload);
  transmitPacket(&pkt);
}

void sendDiscover() {
  LoraPacket pkt = createPacket(PKT_DISCOVER, ADDR_BROADCAST);
  pkt.header.dataLen = 0;
  transmitPacket(&pkt);
}

// ============== Handle Functions ==============

void handlePacket(LoraPacket* pkt, float rssi, float snr) {
  // Store last signal quality
  lastRssi = rssi;
  lastSnr = snr;
  
  // Check if packet is for us or broadcast
  if (pkt->header.dstNode != config.nodeId && pkt->header.dstNode != ADDR_BROADCAST) {
    // Not for us - relay if capable
    if (config.capabilities & CAPA_RELAY) {
      // TODO: implement relay with TTL
      Serial.printf("Relay packet from %d to %d\n", 
                    pkt->header.srcNode, pkt->header.dstNode);
    }
    return;
  }
  
  // Update node tracking
  updateNodeStats(pkt->header.srcNode, rssi, snr, pkt->header.seq);
  
  Serial.printf("RX: type=%d src=%d seq=%d rssi=%.0f snr=%.1f\n",
                pkt->header.type, pkt->header.srcNode,
                pkt->header.seq, rssi, snr);
  
  // Handle by type
  switch (pkt->header.type) {
    case PKT_BEAT:
      handleBeat(pkt);
      break;
    case PKT_HEALTH:
      handleHealth(pkt);
      break;
    case PKT_POSITION:
      handlePosition(pkt);
      break;
    case PKT_ALERT:
      handleAlert(pkt);
      if (pkt->header.flags & FLAG_ACK_REQ) {
        sendAck(pkt->header.srcNode, pkt->header.seq);
      }
      break;
    case PKT_PING:
      handlePing(pkt);
      break;
    case PKT_PONG:
      handlePong(pkt);
      break;
    case PKT_ACK:
      handleAck(pkt);
      break;
    case PKT_ANNOUNCE:
      handleAnnounce(pkt);
      break;
    case PKT_DISCOVER:
      handleDiscover(pkt);
      break;
    default:
      Serial.printf("Unknown packet type: %d\n", pkt->header.type);
      break;
  }
}

void handleBeat(LoraPacket* pkt) {
  BeatPayload* p = (BeatPayload*)pkt->payload;
  
  NodeInfo* node = findNode(pkt->header.srcNode);
  if (node) {
    Serial.printf("Beat from %s: bat=%dmV rssi=%d uptime=%dh\n",
                  node->name, p->battery, p->rssi, p->uptime);
  }
}

void handleHealth(LoraPacket* pkt) {
  HealthPayload* p = (HealthPayload*)pkt->payload;
  
  Serial.printf("Health from %d: bat=%dmV rx=%d tx=%d lost=%d temp=%dC\n",
                pkt->header.srcNode, p->battery, p->rxCount,
                p->txCount, p->lostCount, p->temperature);
}

void handlePosition(LoraPacket* pkt) {
  PositionPayload* p = (PositionPayload*)pkt->payload;
  
  float lat = p->latitude / 1e7;
  float lon = p->longitude / 1e7;
  
  Serial.printf("Position from %d: %.6f, %.6f, %d km/h, %d sats\n",
                pkt->header.srcNode, lat, lon, p->speed, p->satellites);
  
  // TODO: forward to gateway, update map, etc.
}

void handleAlert(LoraPacket* pkt) {
  AlertPayload* p = (AlertPayload*)pkt->payload;
  
  float lat = p->latitude / 1e7;
  float lon = p->longitude / 1e7;
  
  const char* alertName = "UNKNOWN";
  switch (p->alertType) {
    case ALERT_MOTION:  alertName = "MOTION"; break;
    case ALERT_TAMPER:  alertName = "TAMPER"; break;
    case ALERT_BATTERY: alertName = "BATTERY"; break;
    case ALERT_GEOFENCE: alertName = "GEOFENCE"; break;
  }
  
  Serial.printf("!!! ALERT from %d: %s (severity=%d) at %.6f, %.6f\n",
                pkt->header.srcNode, alertName, p->severity, lat, lon);
  
  // TODO: forward to gateway, send notification, etc.
}

void handlePing(LoraPacket* pkt) {
  PingPayload* p = (PingPayload*)pkt->payload;
  
  Serial.printf("Ping from %d, responding...\n", pkt->header.srcNode);
  sendPong(pkt->header.srcNode, p->timestamp);
}

void handlePong(LoraPacket* pkt) {
  PingPayload* p = (PingPayload*)pkt->payload;
  
  uint32_t rtt = millis() - p->timestamp;
  Serial.printf("Pong from %d: RTT=%d ms\n", pkt->header.srcNode, rtt);
}

void handleAck(LoraPacket* pkt) {
  AckPayload* p = (AckPayload*)pkt->payload;
  
  Serial.printf("Ack from %d for seq=%d\n", pkt->header.srcNode, p->ackedSeq);
  
  // TODO: mark packet as acknowledged, stop retries
}

void handleAnnounce(LoraPacket* pkt) {
  AnnouncePayload* p = (AnnouncePayload*)pkt->payload;
  
  NodeInfo* node = findNode(pkt->header.srcNode);
  if (!node) {
    node = addNode(pkt->header.srcNode);
    if (!node) return;
  }
  
  strncpy(node->name, p->name, 12);
  node->name[11] = '\0';
  node->capabilities = p->capabilities;
  
  Serial.printf("Announce from %d: name=%s capa=0x%02X bat=%dmV\n",
                pkt->header.srcNode, node->name, p->capabilities, p->battery);
}

void handleDiscover(LoraPacket* pkt) {
  Serial.printf("Discover from %d, announcing...\n", pkt->header.srcNode);
  
  // Random delay to avoid collisions
  delay(random(100, 500));
  sendAnnounce();
}

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
  node->nodeId = nodeId;
  node->lastSeq = -1;
  sprintf(node->name, "Node%d", nodeId);
  
  Serial.printf("Added new node: %d\n", nodeId);
  return node;
}

void updateNodeStats(uint8_t nodeId, float rssi, float snr, int seq) {
  NodeInfo* node = findNode(nodeId);
  if (!node) {
    node = addNode(nodeId);
    if (!node) return;
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
  
  node->lastSeq = seq;
  node->lastSeen = millis();
  node->lastRssi = rssi;
  node->lastSnr = snr;
  node->rxCount++;
}

void printNodes() {
  Serial.println("\n=== Known Nodes ===");
  for (int i = 0; i < nodeCount; i++) {
    NodeInfo* n = &knownNodes[i];
    int total = n->rxCount + n->lostCount;
    int lossPct = (total > 0) ? (100 * n->lostCount / total) : 0;
    
    Serial.printf("[%d] %s: rx=%d lost=%d (%d%%) rssi=%.0f snr=%.1f last=%ds ago\n",
                  n->nodeId, n->name, n->rxCount, n->lostCount, lossPct,
                  n->lastRssi, n->lastSnr, (millis() - n->lastSeen) / 1000);
  }
  Serial.println("===================\n");
}