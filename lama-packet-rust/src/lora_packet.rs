// Constants from lora_packet.h
const LORA_PACKET_MAGIC: u8 = 0x42;
const LORA_PACKET_VERSION: u8 = 1;
const LORA_PACKET_MAXLENGTH: usize = 64;
const LORA_MAX_PAYLOAD: u8 = 32;
const LORA_HEADER_SIZE: usize = 8;
const LORA_BEAT_PAYLOAD_SIZE: usize = 6;

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum LoraPacketType {
    Empty = 0x00,
    Beat = 0x01,
    Health = 0x02,
    Config = 0x03,
    Ping = 0x04,
    Pong = 0x05,
    Data = 0x06,
    Ack = 0x07,
    Alert = 0x08,
    Position = 0x09,
    Discover = 0x0A,
    Announce = 0x0B,
}

impl LoraPacketType {
    fn from_u8(value: u8) -> Option<Self> {
        match value {
            0x00 => Some(LoraPacketType::Empty),
            0x01 => Some(LoraPacketType::Beat),
            0x02 => Some(LoraPacketType::Health),
            0x03 => Some(LoraPacketType::Config),
            0x04 => Some(LoraPacketType::Ping),
            0x05 => Some(LoraPacketType::Pong),
            0x06 => Some(LoraPacketType::Data),
            0x07 => Some(LoraPacketType::Ack),
            0x08 => Some(LoraPacketType::Alert),
            0x09 => Some(LoraPacketType::Position),
            0x0A => Some(LoraPacketType::Discover),
            0x0B => Some(LoraPacketType::Announce),
            _ => None,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct LoraPacketNode {
    pub node: u8,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct LoraPacketHeader {
    pub magic: u8,
    pub version: u8,
    pub pkt_type: u8,
    pub seq: u16,
    pub src: LoraPacketNode,
    pub payload_len: u8,
}

#[repr(C)]
pub struct LoraPacket {
    pub header: LoraPacketHeader,
    pub payload: [u8; LORA_MAX_PAYLOAD as usize],
}

/// Validate a LoRa packet from raw bytes
///
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure that `data` points to valid memory of at least `len` bytes.
pub fn validate_packet(data: &[u8]) -> bool {
    let len = data.len();

    // Minimum size check
    if len < LORA_HEADER_SIZE {
        return false;
    }

    // Maximum size check
    if len > LORA_PACKET_MAXLENGTH {
        return false;
    }

    // Check magic number FIRST - don't trust other fields if magic is wrong
    if data[0] != LORA_PACKET_MAGIC {
        return false;
    }

    // Check version
    if data[1] != LORA_PACKET_VERSION {
        return false;
    }

    // Validate packet type
    let pkt_type = data[2];
    if LoraPacketType::from_u8(pkt_type).is_none() {
        return false;
    }

    // Get payload length (at offset 7 due to struct alignment)
    let payload_len = data[7];

    // Payload size sanity
    if payload_len > LORA_MAX_PAYLOAD {
        return false;
    }

    // Validate actual packet size matches header + declared payload
    let expected_len = LORA_HEADER_SIZE + payload_len as usize;
    if len != expected_len {
        return false;
    }

    // Type-specific payload validation
    if pkt_type == LoraPacketType::Beat as u8 && payload_len as usize != LORA_BEAT_PAYLOAD_SIZE {
        return false;
    }

    true
}

/// FFI wrapper for validate_packet
///
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure that `data` points to valid memory of at least `len` bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn validate_lora_packet(data: *const u8, len: usize) -> bool {
    if data.is_null() {
        return false;
    }

    // Create a slice from the raw pointer
    let slice = unsafe { core::slice::from_raw_parts(data, len) };

    validate_packet(slice)
}

/// Parses a LoRa packet from a raw byte buffer.
///
/// Returns a tuple of (success, packet)
/// Returns `true` if parsing succeeded and `out_packet` was populated,
/// otherwise returns `false`.
///
/// # Safety
///
/// The caller must guarantee that:
///
/// - `data` points to a valid memory region of at least `len` bytes
/// - `data` is readable for the duration of the call
/// - `out_packet` is a valid, non-null pointer to writable memory
/// - `out_packet` points to properly aligned storage for `LoraPacket`
///
/// Violating any of these requirements results in undefined behavior.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn parse_lora_packet(
    data: *const u8,
    len: usize,
    out_packet: *mut LoraPacket,
) -> bool {
    if data.is_null() || out_packet.is_null() {
        return false;
    }

    let slice = unsafe { core::slice::from_raw_parts(data, len) };

    // Validate first
    if !validate_packet(slice) {
        return false;
    }

    // Parse header (with struct alignment padding after pkt_type)
    let header = LoraPacketHeader {
        magic: slice[0],
        version: slice[1],
        pkt_type: slice[2],
        seq: u16::from_le_bytes([slice[4], slice[5]]),
        src: LoraPacketNode { node: slice[6] },
        payload_len: slice[7],
    };

    // Parse payload
    let mut payload = [0u8; LORA_MAX_PAYLOAD as usize];
    let payload_len = header.payload_len as usize;
    if payload_len > 0 {
        payload[..payload_len]
            .copy_from_slice(&slice[LORA_HEADER_SIZE..LORA_HEADER_SIZE + payload_len]);
    }

    // Write to output
    unsafe {
        (*out_packet).header = header;
        (*out_packet).payload = payload;
    }

    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_validate_valid_beat_packet() {
        // Valid BEAT packet with 6-byte payload
        let packet = [
            0x42, // magic (offset 0)
            0x01, // version (offset 1)
            0x01, // type (BEAT) (offset 2)
            0x00, // padding (offset 3)
            0x01, 0x00, // seq (little-endian u16) (offset 4-5)
            0x0A, // src node (offset 6)
            0x06, // payload_len (offset 7)
            // Payload (6 bytes for BEAT)
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        ];

        assert!(validate_packet(&packet));
    }

    #[test]
    fn test_validate_invalid_magic() {
        let packet = [
            0x99, // wrong magic
            0x01, 0x01, 0x00, 0x01, 0x00, 0x0A, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_invalid_version() {
        let packet = [
            0x42, // magic
            0x99, // wrong version
            0x01, 0x00, 0x01, 0x00, 0x0A, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_too_short() {
        let packet = [0x42, 0x01, 0x01]; // Only 3 bytes

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_too_long() {
        let mut packet = [0u8; 65]; // 65 bytes, exceeds max
        packet[0] = 0x42;
        packet[1] = 0x01;
        packet[2] = 0x01;

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_invalid_packet_type() {
        let packet = [
            0x42, // magic
            0x01, // version
            0xFF, // invalid type
            0x00, // padding
            0x01, 0x00, 0x0A, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_payload_too_large() {
        let packet = [
            0x42, // magic
            0x01, // version
            0x01, // type
            0x00, // padding
            0x01, 0x00, 0x0A, 0xFF, // payload_len = 255 (exceeds MAX_PAYLOAD)
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_beat_wrong_payload_size() {
        // BEAT packet must have exactly 6-byte payload
        let packet = [
            0x42, // magic
            0x01, // version
            0x01, // type (BEAT)
            0x00, // padding
            0x01, 0x00, 0x0A, 0x04, // payload_len = 4 (should be 6)
            0x01, 0x02, 0x03, 0x04,
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_validate_length_mismatch() {
        // Header says 6 bytes payload, but we only provide 4
        let packet = [
            0x42, 0x01, 0x01, 0x00, 0x01, 0x00, 0x0A, 0x06, // claims 6 bytes
            0x01, 0x02, 0x03, 0x04, // only 4 bytes
        ];

        assert!(!validate_packet(&packet));
    }

    #[test]
    fn test_parse_valid_packet() {
        let packet_data = [
            0x42, // magic
            0x01, // version
            0x01, // type (BEAT)
            0x00, // padding
            0x2A, 0x00, // seq = 42
            0x0F, // src node = 15
            0x06, // payload_len = 6
            // Payload
            0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        ];

        let mut packet = LoraPacket {
            header: LoraPacketHeader {
                magic: 0,
                version: 0,
                pkt_type: 0,
                seq: 0,
                src: LoraPacketNode { node: 0 },
                payload_len: 0,
            },
            payload: [0u8; LORA_MAX_PAYLOAD as usize],
        };

        let result = unsafe {
            parse_lora_packet(
                packet_data.as_ptr(),
                packet_data.len(),
                &mut packet as *mut LoraPacket,
            )
        };

        assert!(result);
        assert_eq!(packet.header.magic, 0x42);
        assert_eq!(packet.header.version, 0x01);
        assert_eq!(packet.header.pkt_type, 0x01);
        assert_eq!(packet.header.seq, 42);
        assert_eq!(packet.header.src.node, 0x0F);
        assert_eq!(packet.header.payload_len, 6);
        assert_eq!(packet.payload[0], 0xAA);
        assert_eq!(packet.payload[5], 0xFF);
    }

    #[test]
    fn test_parse_invalid_packet() {
        let packet_data = [
            0x99, // wrong magic
            0x01, 0x01, 0x00, 0x01, 0x00, 0x0A, 0x06,
        ];

        let mut packet = LoraPacket {
            header: LoraPacketHeader {
                magic: 0,
                version: 0,
                pkt_type: 0,
                seq: 0,
                src: LoraPacketNode { node: 0 },
                payload_len: 0,
            },
            payload: [0u8; LORA_MAX_PAYLOAD as usize],
        };

        let result = unsafe {
            parse_lora_packet(
                packet_data.as_ptr(),
                packet_data.len(),
                &mut packet as *mut LoraPacket,
            )
        };

        assert!(!result);
    }
}
