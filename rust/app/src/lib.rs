//! The LAMA node: ties the pure state machine from `lama-core` to a [`Radio`]
//! and a [`Clock`] from `lama-hal`.
//!
//! The node is a plain event loop. A board (or the simulator) calls
//! [`Node::step`] regularly; the node polls the radio, fires timers, feeds
//! events to the state machine and carries out the actions it returns. It
//! never blocks and never allocates.

#![no_std]
#![forbid(unsafe_code)]

use lama_core::packet::{self, BeatPayload, NodeId, PacketType, MAX_PACKET_LEN};
use lama_core::state::{Action, Event, State, StateMachine};
use lama_core::stats::NodeTable;
use lama_hal::{Clock, Radio, RadioEvent, RadioParams};

/// Node configuration.
#[derive(Clone, Copy, Debug)]
pub struct Config {
    pub node_id: NodeId,
    pub radio: RadioParams,
    /// Time between beacons.
    pub beacon_interval_ms: u32,
    /// Delay before retrying a failed radio start.
    pub retry_delay_ms: u32,
}

/// A LAMA node tracking up to `N` remote nodes.
pub struct Node<R: Radio, C: Clock, const N: usize> {
    radio: R,
    clock: C,
    cfg: Config,
    sm: StateMachine,
    table: NodeTable<N>,
    seq: u16,
    last_beacon_ms: u32,
    retry_at_ms: Option<u32>,
    last_snr_cdb: i16,
    started: bool,
    tx_count: u32,
    rx_dropped: u32,
}

impl<R: Radio, C: Clock, const N: usize> Node<R, C, N> {
    pub fn new(radio: R, clock: C, cfg: Config) -> Self {
        Self {
            radio,
            clock,
            cfg,
            sm: StateMachine::new(),
            table: NodeTable::new(),
            seq: 0,
            last_beacon_ms: 0,
            retry_at_ms: None,
            last_snr_cdb: 0,
            started: false,
            tx_count: 0,
            rx_dropped: 0,
        }
    }

    pub fn node_id(&self) -> NodeId {
        self.cfg.node_id
    }

    pub fn state(&self) -> State {
        self.sm.state()
    }

    /// Link statistics for every remote node heard so far.
    pub fn table(&self) -> &NodeTable<N> {
        &self.table
    }

    /// Beacons handed to the radio.
    pub fn tx_count(&self) -> u32 {
        self.tx_count
    }

    /// Received frames that failed validation.
    pub fn rx_dropped(&self) -> u32 {
        self.rx_dropped
    }

    pub fn radio(&self) -> &R {
        &self.radio
    }

    /// Runs one pass of the event loop. Call it often (every tick or main-loop turn).
    pub fn step(&mut self) {
        let now = self.clock.now_ms();

        if !self.started {
            self.started = true;
            self.last_beacon_ms = now;
            self.dispatch(Event::Booted);
        }

        if let Some(at) = self.retry_at_ms {
            // `now >= at`, tolerant of clock wraparound
            if now.wrapping_sub(at) < 0x8000_0000 {
                self.retry_at_ms = None;
                self.dispatch(Event::RetryElapsed);
            }
        }

        while let Some(ev) = self.radio.poll_event() {
            self.dispatch(match ev {
                RadioEvent::RxReady => Event::RxInterrupt,
                RadioEvent::TxDone => Event::TxDone,
            });
        }

        if now.wrapping_sub(self.last_beacon_ms) >= self.cfg.beacon_interval_ms {
            self.last_beacon_ms = now;
            self.dispatch(Event::TxDue);
        }
    }

    /// Feeds an event to the state machine and runs the resulting actions
    /// (which may produce follow-up events) until the machine settles.
    fn dispatch(&mut self, event: Event) {
        let mut next = Some(event);
        while let Some(e) = next.take() {
            let action = self.sm.handle(e);
            next = self.execute(action);
        }
    }

    fn execute(&mut self, action: Action) -> Option<Event> {
        match action {
            Action::None | Action::Halt => None,
            Action::InitRadio => Some(match self.radio.begin(&self.cfg.radio) {
                Ok(()) => Event::RadioReady,
                Err(_) => Event::Fault,
            }),
            Action::StartRx => self.radio.start_rx().err().map(|_| Event::Fault),
            Action::ReadPacket => Some(match self.read_packet() {
                Ok(()) => Event::RxDone,
                Err(()) => Event::Fault,
            }),
            Action::SendBeat => self.send_beat().err().map(|()| Event::Fault),
            Action::ScheduleRetry => {
                self.retry_at_ms = Some(self.clock.now_ms().wrapping_add(self.cfg.retry_delay_ms));
                None
            }
        }
    }

    fn read_packet(&mut self) -> Result<(), ()> {
        let mut buf = [0u8; MAX_PACKET_LEN];
        let Some((len, meta)) = self.radio.read(&mut buf).map_err(|_| ())? else {
            return Ok(());
        };
        let now = self.clock.now_ms();
        match buf.get(..len).map(packet::decode) {
            Some(Ok(p)) if p.header.src != self.cfg.node_id => {
                // A full table only means we stop tracking new nodes.
                let _ = self.table.record(
                    p.header.src,
                    p.header.seq,
                    meta.rssi_cdbm,
                    meta.snr_cdb,
                    now,
                );
                self.last_snr_cdb = meta.snr_cdb;
            }
            Some(Ok(_)) => {} // our own frame
            _ => self.rx_dropped = self.rx_dropped.saturating_add(1),
        }
        Ok(())
    }

    fn send_beat(&mut self) -> Result<(), ()> {
        let payload = BeatPayload {
            msecs: self.clock.now_ms() as u16, // wraps on purpose
            rsnr_cdb: self.last_snr_cdb,
        }
        .to_bytes();
        let mut buf = [0u8; MAX_PACKET_LEN];
        let n = packet::encode(
            PacketType::Beat,
            self.seq,
            self.cfg.node_id,
            &payload,
            &mut buf,
        )
        .map_err(|_| ())?;
        self.radio.transmit(&buf[..n]).map_err(|_| ())?;
        self.seq = self.seq.wrapping_add(1);
        self.tx_count = self.tx_count.saturating_add(1);
        Ok(())
    }
}
