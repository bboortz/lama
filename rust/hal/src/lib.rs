//! Hardware abstraction traits.
//!
//! The traits are synchronous and non-blocking so that the node logic is a
//! plain event loop (see `lama-app`). A board crate implements them on top of
//! its chip HAL; interrupt handlers only set flags that [`Radio::poll_event`]
//! reports. Async runtimes (e.g. Embassy) can wrap these calls in tasks.

#![no_std]
#![forbid(unsafe_code)]

/// LoRa modulation and RF settings.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct RadioParams {
    pub freq_hz: u32,
    pub bandwidth_hz: u32,
    /// Spreading factor, 7..=12.
    pub spreading_factor: u8,
    /// Coding rate denominator, 5..=8 (4/5 .. 4/8).
    pub coding_rate: u8,
    pub tx_power_dbm: i8,
    pub sync_word: u8,
    pub preamble_len: u16,
}

impl RadioParams {
    /// Common EU868 starting point (868.1 MHz, SF7, 125 kHz, CR 4/5, 14 dBm).
    pub const EU868_DEFAULT: Self = Self {
        freq_hz: 868_100_000,
        bandwidth_hz: 125_000,
        spreading_factor: 7,
        coding_rate: 5,
        tx_power_dbm: 14,
        sync_word: 0x12,
        preamble_len: 8,
    };
}

/// Link quality of a received frame, in centi-units (1/100 dBm, 1/100 dB).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct RxMeta {
    pub rssi_cdbm: i16,
    pub snr_cdb: i16,
}

/// Something the radio reports after the fact (set by an interrupt on real hardware).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RadioEvent {
    /// Initialisation started by [`Radio::begin`] has finished successfully.
    Ready,
    /// A frame is ready to be read with [`Radio::read`].
    RxReady,
    /// The frame passed to [`Radio::transmit`] has been sent.
    TxDone,
    /// An operation that was accepted earlier failed on the hardware
    /// (initialisation, transmission or reception).
    Error,
}

/// A packet radio. Calls never block; completion is reported via [`Radio::poll_event`].
///
/// Methods return `Err` only when a request cannot even be started (radio busy,
/// frame too long, ...). Hardware failures that happen later are reported as
/// [`RadioEvent::Error`].
pub trait Radio {
    type Error: core::fmt::Debug;

    /// Starts initialising the radio; [`RadioEvent::Ready`] follows when done.
    fn begin(&mut self, params: &RadioParams) -> Result<(), Self::Error>;

    /// Starts (or continues) listening.
    fn start_rx(&mut self) -> Result<(), Self::Error>;

    /// Starts transmitting `frame`; [`RadioEvent::TxDone`] follows when finished.
    fn transmit(&mut self, frame: &[u8]) -> Result<(), Self::Error>;

    /// Returns the next pending radio event, if any.
    fn poll_event(&mut self) -> Option<RadioEvent>;

    /// Reads one received frame into `buf`; returns its length and link quality.
    /// `Ok(None)` means nothing is pending.
    fn read(&mut self, buf: &mut [u8]) -> Result<Option<(usize, RxMeta)>, Self::Error>;
}

/// Monotonic millisecond clock (wraps at `u32::MAX`).
pub trait Clock {
    fn now_ms(&self) -> u32;
}
