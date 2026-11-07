//! Wire format v2: explicit, little-endian, no padding.
//!
//! ```text
//! offset  size  field
//! 0       1     magic (0x42)
//! 1       1     version (2)
//! 2       1     packet type
//! 3       2     sequence number (u16, little-endian)
//! 5       1     source node id
//! 6       1     payload length (0..=32)
//! 7       n     payload
//! ```
//!
//! This is *not* byte-compatible with the C++ firmware, which sends raw
//! structs (including compiler padding). Version 2 makes the layout explicit
//! so every chip and language agrees.

/// Node identifier on the wire.
pub type NodeId = u8;

/// Broadcast address.
pub const BROADCAST: NodeId = 0xFF;

/// First byte of every packet.
pub const MAGIC: u8 = 0x42;
/// Wire format version implemented by this module.
pub const VERSION: u8 = 2;
/// Header length in bytes.
pub const HEADER_LEN: usize = 7;
/// Maximum payload length in bytes.
pub const MAX_PAYLOAD: usize = 32;
/// Maximum length of a whole packet in bytes.
pub const MAX_PACKET_LEN: usize = HEADER_LEN + MAX_PAYLOAD;

/// Packet types (values match the C++ firmware; `0x00` is "empty" and never valid on the wire).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum PacketType {
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

impl PacketType {
    /// All valid packet types.
    pub const ALL: [PacketType; 11] = [
        PacketType::Beat,
        PacketType::Health,
        PacketType::Config,
        PacketType::Ping,
        PacketType::Pong,
        PacketType::Data,
        PacketType::Ack,
        PacketType::Alert,
        PacketType::Position,
        PacketType::Discover,
        PacketType::Announce,
    ];

    /// Converts a wire byte into a packet type.
    pub const fn from_u8(v: u8) -> Option<Self> {
        match v {
            0x01 => Some(Self::Beat),
            0x02 => Some(Self::Health),
            0x03 => Some(Self::Config),
            0x04 => Some(Self::Ping),
            0x05 => Some(Self::Pong),
            0x06 => Some(Self::Data),
            0x07 => Some(Self::Ack),
            0x08 => Some(Self::Alert),
            0x09 => Some(Self::Position),
            0x0A => Some(Self::Discover),
            0x0B => Some(Self::Announce),
            _ => None,
        }
    }
}

/// Decoded packet header.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Header {
    pub ptype: PacketType,
    pub seq: u16,
    pub src: NodeId,
    pub payload_len: u8,
}

/// A decoded packet borrowing its payload from the receive buffer.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Packet<'a> {
    pub header: Header,
    pub payload: &'a [u8],
}

/// Why a buffer could not be decoded.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DecodeError {
    /// Fewer bytes than a header.
    TooShort,
    BadMagic,
    BadVersion(u8),
    UnknownType(u8),
    /// Declared payload length exceeds [`MAX_PAYLOAD`].
    PayloadTooLong,
    /// Buffer length does not equal header plus declared payload length.
    LengthMismatch,
    /// Payload has the wrong size or shape for its type.
    BadPayload,
}

/// Why a packet could not be encoded.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum EncodeError {
    PayloadTooLong,
    BufferTooSmall,
}

/// Encodes a packet into `out` and returns the number of bytes written.
pub fn encode(
    ptype: PacketType,
    seq: u16,
    src: NodeId,
    payload: &[u8],
    out: &mut [u8],
) -> Result<usize, EncodeError> {
    if payload.len() > MAX_PAYLOAD {
        return Err(EncodeError::PayloadTooLong);
    }
    let total = HEADER_LEN + payload.len();
    if out.len() < total {
        return Err(EncodeError::BufferTooSmall);
    }
    let s = seq.to_le_bytes();
    out[0] = MAGIC;
    out[1] = VERSION;
    out[2] = ptype as u8;
    out[3] = s[0];
    out[4] = s[1];
    out[5] = src;
    out[6] = payload.len() as u8; // <= MAX_PAYLOAD, checked above
    out[HEADER_LEN..total].copy_from_slice(payload);
    Ok(total)
}

/// Decodes and validates a received buffer.
pub fn decode(buf: &[u8]) -> Result<Packet<'_>, DecodeError> {
    if buf.len() < HEADER_LEN {
        return Err(DecodeError::TooShort);
    }
    if buf[0] != MAGIC {
        return Err(DecodeError::BadMagic);
    }
    if buf[1] != VERSION {
        return Err(DecodeError::BadVersion(buf[1]));
    }
    let ptype = PacketType::from_u8(buf[2]).ok_or(DecodeError::UnknownType(buf[2]))?;
    let seq = u16::from_le_bytes([buf[3], buf[4]]);
    let src = buf[5];
    let len = usize::from(buf[6]);
    if len > MAX_PAYLOAD {
        return Err(DecodeError::PayloadTooLong);
    }
    if buf.len() != HEADER_LEN + len {
        return Err(DecodeError::LengthMismatch);
    }
    Ok(Packet {
        header: Header {
            ptype,
            seq,
            src,
            payload_len: buf[6],
        },
        payload: &buf[HEADER_LEN..],
    })
}

/// Payload of a [`PacketType::Beat`] packet (4 bytes).
///
/// `msecs` is a sender-defined millisecond counter (as in the C++ firmware;
/// its exact meaning is pinned down when the beat service is ported).
/// `rsnr_cdb` is the SNR the sender last measured for the receiver, in
/// centi-dB (1/100 dB), fixed-point instead of a float on the wire.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BeatPayload {
    pub msecs: u16,
    pub rsnr_cdb: i16,
}

impl BeatPayload {
    /// Encoded length in bytes.
    pub const LEN: usize = 4;

    pub fn to_bytes(self) -> [u8; Self::LEN] {
        let m = self.msecs.to_le_bytes();
        let r = self.rsnr_cdb.to_le_bytes();
        [m[0], m[1], r[0], r[1]]
    }

    pub fn from_bytes(b: &[u8]) -> Result<Self, DecodeError> {
        if b.len() != Self::LEN {
            return Err(DecodeError::BadPayload);
        }
        Ok(Self {
            msecs: u16::from_le_bytes([b[0], b[1]]),
            rsnr_cdb: i16::from_le_bytes([b[2], b[3]]),
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn golden_bytes() {
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Ping, 0x0102, 0x07, &[0xAA, 0xBB], &mut out).unwrap();
        assert_eq!(&out[..n], &[0x42, 2, 0x04, 0x02, 0x01, 0x07, 2, 0xAA, 0xBB]);
    }

    #[test]
    fn header_is_seven_bytes() {
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Beat, 0, 1, &[], &mut out).unwrap();
        assert_eq!(n, HEADER_LEN);
        assert_eq!(HEADER_LEN, 7);
    }

    #[test]
    fn roundtrip_all_types_and_all_sequence_numbers() {
        let mut out = [0u8; MAX_PACKET_LEN];
        for ptype in PacketType::ALL {
            for seq in 0..=u16::MAX {
                let n = encode(ptype, seq, 9, &[1, 2, 3], &mut out).unwrap();
                let p = decode(&out[..n]).unwrap();
                assert_eq!(p.header.ptype, ptype);
                assert_eq!(p.header.seq, seq);
                assert_eq!(p.header.src, 9);
                assert_eq!(p.payload, &[1, 2, 3]);
            }
        }
    }

    #[test]
    fn roundtrip_max_payload() {
        let payload = [0x5A; MAX_PAYLOAD];
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Data, 1, BROADCAST, &payload, &mut out).unwrap();
        assert_eq!(n, MAX_PACKET_LEN);
        assert_eq!(decode(&out[..n]).unwrap().payload, &payload);
    }

    #[test]
    fn encode_rejects_oversized_payload_and_small_buffer() {
        let mut out = [0u8; MAX_PACKET_LEN];
        let big = [0u8; MAX_PAYLOAD + 1];
        assert_eq!(
            encode(PacketType::Data, 0, 1, &big, &mut out),
            Err(EncodeError::PayloadTooLong)
        );
        let mut small = [0u8; HEADER_LEN + 1];
        assert_eq!(
            encode(PacketType::Data, 0, 1, &[1, 2], &mut small),
            Err(EncodeError::BufferTooSmall)
        );
    }

    #[test]
    fn decode_rejects_bad_input() {
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Pong, 5, 2, &[1], &mut out).unwrap();
        let good = out;

        assert_eq!(decode(&good[..HEADER_LEN - 1]), Err(DecodeError::TooShort));
        assert_eq!(decode(&[]), Err(DecodeError::TooShort));

        let mut b = good;
        b[0] = 0x00;
        assert_eq!(decode(&b[..n]), Err(DecodeError::BadMagic));

        let mut b = good;
        b[1] = 1;
        assert_eq!(decode(&b[..n]), Err(DecodeError::BadVersion(1)));

        let mut b = good;
        b[2] = 0x00;
        assert_eq!(decode(&b[..n]), Err(DecodeError::UnknownType(0x00)));
        b[2] = 0x7F;
        assert_eq!(decode(&b[..n]), Err(DecodeError::UnknownType(0x7F)));

        let mut b = good;
        b[6] = 33;
        assert_eq!(decode(&b[..n]), Err(DecodeError::PayloadTooLong));

        // declared length 1, but two bytes of payload / none at all
        assert_eq!(decode(&good[..n - 1]), Err(DecodeError::LengthMismatch));
        assert_eq!(decode(&good[..n + 1]), Err(DecodeError::LengthMismatch));
    }

    #[test]
    fn decode_never_panics_on_any_single_byte_mutation() {
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Beat, 0x1234, 3, &[9, 8, 7, 6], &mut out).unwrap();
        for i in 0..n {
            for v in 0..=u8::MAX {
                let mut b = out;
                b[i] = v;
                let _ = decode(&b[..n]);
            }
        }
        for len in 0..=n + 2 {
            let _ = decode(&out[..len.min(out.len())]);
        }
    }

    #[test]
    fn beat_payload_roundtrip_including_negative_snr() {
        for (msecs, rsnr_cdb) in [
            (0, 0),
            (65535, -2000),
            (1234, 1025),
            (1, i16::MIN),
            (2, i16::MAX),
        ] {
            let p = BeatPayload { msecs, rsnr_cdb };
            assert_eq!(BeatPayload::from_bytes(&p.to_bytes()), Ok(p));
        }
        assert_eq!(
            BeatPayload::from_bytes(&[1, 2, 3]),
            Err(DecodeError::BadPayload)
        );
        assert_eq!(
            BeatPayload::from_bytes(&[1, 2, 3, 4, 5]),
            Err(DecodeError::BadPayload)
        );
    }

    #[test]
    fn beat_packet_end_to_end() {
        let beat = BeatPayload {
            msecs: 500,
            rsnr_cdb: -725,
        };
        let mut out = [0u8; MAX_PACKET_LEN];
        let n = encode(PacketType::Beat, 42, 4, &beat.to_bytes(), &mut out).unwrap();
        let p = decode(&out[..n]).unwrap();
        assert_eq!(BeatPayload::from_bytes(p.payload), Ok(beat));
    }
}
