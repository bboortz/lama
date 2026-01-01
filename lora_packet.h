#ifndef LORA_PACKET_H
#define LORA_PACKET_H

#define LORA_PACKET_MAGIC 0x42
#define LORA_PACKET_MAXLENGTH 64
#define LORA_PACKET_VERSION 1
#define LORA_MAX_PAYLOAD 32

// Packet types (1 Byte)
enum LoraPacketType : uint8_t {
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

// Packet Node (1 bytes)
struct LoraPacketNode {
  // uint8_t  networkId;
  uint8_t node;
};

// Packet header (8 bytes)
// Due to struct alignment on ESP32, the header is 8 bytes, not 7.
struct LoraPacketHeader {
  uint8_t        magic;
  uint8_t        version;
  LoraPacketType type;
  uint16_t       seq;
  // uint8_t  flags;
  LoraPacketNode src;
  // LoraPacketNode dst;
  uint8_t payloadLen;
};

// Packet Beat Payload (6 Bytes)
struct LoraPacketBeatPayload {
  // String user;
  // int    seq;
  uint16_t msecs;
  float    rsnr;
};

struct LoraPacket {
  LoraPacketHeader header;
  // LoraPacketPayload payload;
  uint8_t payload[LORA_MAX_PAYLOAD];
};

// Payloads
/*
struct BeatPayload {
  uint16_t battery;
  int8_t   rssi;
  uint8_t  uptime;
};
*/

constexpr size_t LORA_HEADER_SIZE       = sizeof(LoraPacketHeader);
constexpr size_t LORA_BEAT_PAYLOAD_SIZE = sizeof(LoraPacketBeatPayload);
constexpr size_t LORA_BEAT_PACKET_SIZE  = LORA_HEADER_SIZE + LORA_BEAT_PAYLOAD_SIZE;
constexpr size_t LORA_MAX_PACKET_SIZE   = LORA_HEADER_SIZE + LORA_MAX_PAYLOAD;

#endif