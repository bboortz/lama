#ifndef LAMA_PACKET_FFI_H
#define LAMA_PACKET_FFI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Test function to verify FFI linkage
bool lama_packet_callable(void);

// Validate a LoRa packet from raw bytes
//
// Returns true if the packet is valid, false otherwise
//
// # Safety
// The caller must ensure that `data` points to valid memory of at least `len` bytes.
bool validate_lora_packet(const uint8_t* data, size_t len);

// LoRa packet structures matching Rust #[repr(C)] layout
// Note: These must match the struct alignment from the Rust side

typedef struct {
    uint8_t node;
} LoraPacketNode;

typedef struct {
    uint8_t        magic;       // offset 0
    uint8_t        version;     // offset 1
    uint8_t        pkt_type;    // offset 2
    uint8_t        _padding;    // offset 3 (alignment padding)
    uint16_t       seq;         // offset 4-5
    LoraPacketNode src;         // offset 6
    uint8_t        payload_len; // offset 7
} LoraPacketHeader;

typedef struct {
    LoraPacketHeader header;
    uint8_t          payload[32]; // LORA_MAX_PAYLOAD
} LoraPacket;

// Parse a LoRa packet from raw bytes
//
// Returns true if the packet was successfully parsed, false otherwise
// If successful, the parsed packet is written to `out_packet`
//
// # Safety
// The caller must ensure that:
// - `data` points to valid memory of at least `len` bytes
// - `out_packet` points to valid writable memory
bool parse_lora_packet(const uint8_t* data, size_t len, LoraPacket* out_packet);

#ifdef __cplusplus
}
#endif

#endif // LAMA_PACKET_FFI_H
