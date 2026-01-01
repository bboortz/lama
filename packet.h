#ifndef PACKET_H
#define PACKET_H

#include <Arduino.h>

// Packet flags
enum PacketFlags : uint8_t {
  FLAG_NONE       = 0x00,
  FLAG_ACK_REQ    = 0x01,  // Request acknowledgment
  FLAG_RELAYED    = 0x02,  // Packet was relayed
  FLAG_ENCRYPTED  = 0x04,  // Payload is encrypted
  FLAG_COMPRESSED = 0x08,  // Payload is compressed
};

// Packet types
enum PacketType : uint8_t {
  PKT_EMPTY    = 0x00,
  PKT_BEAT     = 0x01,
  PKT_HEALTH   = 0x02,
  PKT_CONFIG   = 0x03,
  PKT_PING     = 0x04,
  PKT_PONG     = 0x05,
  PKT_DATA     = 0x06,
  PKT_ACK      = 0x07,
  PKT_ALERT    = 0x08,
  PKT_POSITION = 0x09,
  PKT_DISCOVER = 0x0A,
  PKT_ANNOUNCE = 0x0B,
};

// Alert types
enum AlertType : uint8_t {
  ALERT_MOTION   = 0x01,
  ALERT_TAMPER   = 0x02,
  ALERT_BATTERY  = 0x03,
  ALERT_GEOFENCE = 0x04,
};

// Node capabilities bitmask
enum NodeCapa : uint8_t {
  CAPA_RX      = 0x01,
  CAPA_TX      = 0x02,
  CAPA_RELAY   = 0x04,
  CAPA_GATEWAY = 0x08,
  CAPA_GPS     = 0x10,
  CAPA_SENSORS = 0x20,
  CAPA_BATTERY = 0x40,
  CAPA_MAINS   = 0x80,
};

// Broadcast address
#define ADDR_BROADCAST 0xFF

// Packet header (8 bytes)
struct PacketHeader {
  uint8_t  version : 4;
  uint8_t  type : 4;
  uint8_t  networkId;
  uint8_t  srcNode;
  uint8_t  dstNode;
  uint16_t seq;
  uint8_t  flags;
  uint8_t  dataLen;
};

// Payloads
struct BeatPayload {
  uint16_t battery;
  int8_t   rssi;
  uint8_t  uptime;
};

struct HealthPayload {
  uint16_t battery;
  uint16_t rxCount;
  uint16_t txCount;
  uint8_t  lostCount;
  int8_t   temperature;
};

struct PositionPayload {
  int32_t latitude;
  int32_t longitude;
  uint8_t satellites;
  uint8_t speed;
  uint8_t heading;
  uint8_t altitude;
};

struct DataPayload {
  uint8_t sensorType;
  uint8_t sensorId;
  uint8_t data[30];
};

struct AlertPayload {
  uint8_t  alertType;
  uint8_t  severity;
  uint16_t battery;
  int32_t  latitude;
  int32_t  longitude;
};

struct AnnouncePayload {
  char     name[12];
  uint8_t  capabilities;
  uint8_t  version;
  uint16_t battery;
};

struct AckPayload {
  uint16_t ackedSeq;
};

struct PingPayload {
  uint32_t timestamp;
};

// Complete packet
#define MAX_PAYLOAD 32

struct LoraPacket {
  PacketHeader header;
  uint8_t      payload[MAX_PAYLOAD];
};

// Node info for tracking
#define MAX_NODES 16

struct NodeInfo {
  uint8_t  nodeId;
  char     name[12];
  uint8_t  capabilities;
  uint32_t lastSeen;
  float    lastRssi;
  float    lastSnr;
  uint16_t rxCount;
  uint16_t lostCount;
  int      lastSeq;
};

// Function declarations
LoraPacket createPacket(PacketType type, uint8_t dst, uint8_t flags = FLAG_NONE);
int        transmitPacket(LoraPacket* pkt);
void       handlePacket(LoraPacket* pkt, float rssi, float snr);

void sendBeat();
void sendHealth();
void sendPosition();
void sendAlert(uint8_t alertType, uint8_t severity);
void sendPing(uint8_t dstNode);
void sendPong(uint8_t dstNode, uint32_t timestamp);
void sendAck(uint8_t dstNode, uint16_t ackedSeq);
void sendAnnounce();
void sendDiscover();

void handleBeat(LoraPacket* pkt);
void handleHealth(LoraPacket* pkt);
void handlePosition(LoraPacket* pkt);
void handleAlert(LoraPacket* pkt);
void handlePing(LoraPacket* pkt);
void handlePong(LoraPacket* pkt);
void handleAck(LoraPacket* pkt);
void handleAnnounce(LoraPacket* pkt);
void handleDiscover(LoraPacket* pkt);

NodeInfo* findNode(uint8_t nodeId);
NodeInfo* addNode(uint8_t nodeId);
void      updateNodeStats(uint8_t nodeId, float rssi, float snr, int seq);

#endif