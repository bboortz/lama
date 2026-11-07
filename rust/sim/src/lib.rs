//! Deterministic LoRa network simulator.
//!
//! Runs the real `lama-app` [`Node`] code against a simulated radio medium on
//! a virtual clock, one millisecond per tick. The medium models:
//!
//! * airtime (a frame is delivered when it has finished sending),
//! * half-duplex radios (a transmitting node hears nothing),
//! * a receiver must have been listening since before the frame started,
//! * collisions (two audible frames overlapping at a receiver destroy both),
//! * per-link RSSI/SNR and random loss from a seeded PRNG,
//! * fault injection (radio initialisation failures).
//!
//! The same seed always produces the same result.

use std::cell::{Cell, RefCell};
use std::collections::{HashMap, VecDeque};
use std::rc::Rc;

use lama_app::{Config, Node};
use lama_core::packet::NodeId;
use lama_hal::{Clock, Radio, RadioEvent, RadioParams, RxMeta};

/// How many remote nodes each simulated node can track.
pub const TABLE_CAP: usize = 8;

/// Clock shared by all simulated nodes.
#[derive(Clone)]
pub struct SimClock(Rc<Cell<u32>>);

impl Clock for SimClock {
    fn now_ms(&self) -> u32 {
        self.0.get()
    }
}

/// One direction of a radio link.
#[derive(Clone, Copy, Debug)]
pub struct Link {
    pub rssi_cdbm: i16,
    pub snr_cdb: i16,
    /// Probability of losing a frame on this link, in permille.
    pub loss_permille: u16,
}

impl Link {
    /// A clean link with a decent signal.
    pub const GOOD: Link = Link {
        rssi_cdbm: -8500,
        snr_cdb: 800,
        loss_permille: 0,
    };
}

#[derive(Debug, PartialEq, Eq)]
pub enum SimError {
    NotInitialised,
    InitFailed,
    Busy,
}

#[derive(Default)]
struct RadioState {
    begun: bool,
    listening_since: Option<u32>,
    tx_until: Option<u32>,
    events: VecDeque<RadioEvent>,
    rx_queue: VecDeque<(Vec<u8>, RxMeta)>,
    fail_begin: u32,
}

struct Frame {
    src: usize,
    data: Vec<u8>,
    start: u32,
    end: u32,
    delivered: bool,
}

struct Medium {
    clock: Rc<Cell<u32>>,
    radios: Vec<RadioState>,
    links: HashMap<(usize, usize), Link>,
    frames: Vec<Frame>,
    rng: u64,
    airtime_ms: u32,
}

impl Medium {
    fn next_rand(&mut self) -> u64 {
        // xorshift64*
        let mut x = self.rng;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        self.rng = x;
        x.wrapping_mul(0x2545_F491_4F6C_DD1D)
    }

    /// Reports finished transmissions and delivers finished frames.
    fn advance(&mut self) {
        let now = self.clock.get();
        for r in &mut self.radios {
            if r.tx_until.is_some_and(|t| t <= now) {
                r.tx_until = None;
                r.events.push_back(RadioEvent::TxDone);
            }
        }
        let finished: Vec<usize> = self
            .frames
            .iter()
            .enumerate()
            .filter(|(_, f)| !f.delivered && f.end <= now)
            .map(|(i, _)| i)
            .collect();
        for fi in finished {
            self.frames[fi].delivered = true;
            self.deliver(fi);
        }
    }

    fn deliver(&mut self, fi: usize) {
        let (src, start, end) = {
            let f = &self.frames[fi];
            (f.src, f.start, f.end)
        };
        let data = self.frames[fi].data.clone();

        for dst in 0..self.radios.len() {
            if dst == src {
                continue;
            }
            let Some(link) = self.links.get(&(src, dst)).copied() else {
                continue; // out of range
            };
            match self.radios[dst].listening_since {
                Some(t) if t <= start => {}
                _ => continue, // not listening for the whole frame
            }
            let overlaps = |f: &Frame| f.start < end && f.end > start;
            // half duplex: dst was transmitting during the frame
            if self.frames.iter().any(|f| f.src == dst && overlaps(f)) {
                continue;
            }
            // collision with another frame audible at dst
            let collided = self.frames.iter().enumerate().any(|(i, f)| {
                i != fi && f.src != dst && overlaps(f) && self.links.contains_key(&(f.src, dst))
            });
            if collided {
                continue;
            }
            if self.next_rand() % 1000 < u64::from(link.loss_permille) {
                continue;
            }
            let meta = RxMeta {
                rssi_cdbm: link.rssi_cdbm,
                snr_cdb: link.snr_cdb,
            };
            let r = &mut self.radios[dst];
            r.rx_queue.push_back((data.clone(), meta));
            r.events.push_back(RadioEvent::RxReady);
        }
    }
}

/// A node's handle to the shared medium; implements [`Radio`].
pub struct SimRadio {
    id: usize,
    medium: Rc<RefCell<Medium>>,
}

impl Radio for SimRadio {
    type Error = SimError;

    fn begin(&mut self, _params: &RadioParams) -> Result<(), SimError> {
        let mut m = self.medium.borrow_mut();
        let r = &mut m.radios[self.id];
        if r.fail_begin > 0 {
            r.fail_begin -= 1;
            return Err(SimError::InitFailed);
        }
        r.begun = true;
        Ok(())
    }

    fn start_rx(&mut self) -> Result<(), SimError> {
        let mut m = self.medium.borrow_mut();
        let now = m.clock.get();
        let r = &mut m.radios[self.id];
        if !r.begun {
            return Err(SimError::NotInitialised);
        }
        if r.tx_until.is_some() {
            return Err(SimError::Busy);
        }
        r.listening_since.get_or_insert(now);
        Ok(())
    }

    fn transmit(&mut self, frame: &[u8]) -> Result<(), SimError> {
        let mut m = self.medium.borrow_mut();
        let start = m.clock.get();
        let end = start + m.airtime_ms;
        let r = &mut m.radios[self.id];
        if !r.begun {
            return Err(SimError::NotInitialised);
        }
        if r.tx_until.is_some() {
            return Err(SimError::Busy);
        }
        r.listening_since = None;
        r.tx_until = Some(end);
        m.frames.push(Frame {
            src: self.id,
            data: frame.to_vec(),
            start,
            end,
            delivered: false,
        });
        Ok(())
    }

    fn poll_event(&mut self) -> Option<RadioEvent> {
        self.medium.borrow_mut().radios[self.id].events.pop_front()
    }

    fn read(&mut self, buf: &mut [u8]) -> Result<Option<(usize, RxMeta)>, SimError> {
        let mut m = self.medium.borrow_mut();
        Ok(m.radios[self.id].rx_queue.pop_front().map(|(data, meta)| {
            let n = data.len().min(buf.len());
            buf[..n].copy_from_slice(&data[..n]);
            (n, meta)
        }))
    }
}

/// A simulated node.
pub type SimNode = Node<SimRadio, SimClock, TABLE_CAP>;

/// A network of simulated nodes sharing one radio medium.
pub struct Network {
    clock: Rc<Cell<u32>>,
    medium: Rc<RefCell<Medium>>,
    nodes: Vec<SimNode>,
}

impl Network {
    /// `airtime_ms` is how long every frame occupies the channel.
    pub fn new(seed: u64, airtime_ms: u32) -> Self {
        let clock = Rc::new(Cell::new(0));
        let medium = Rc::new(RefCell::new(Medium {
            clock: clock.clone(),
            radios: Vec::new(),
            links: HashMap::new(),
            frames: Vec::new(),
            rng: seed | 1, // xorshift must not start at zero
            airtime_ms,
        }));
        Self {
            clock,
            medium,
            nodes: Vec::new(),
        }
    }

    /// Default node configuration: EU868 parameters, 500 ms radio retry.
    pub fn config(node_id: NodeId, beacon_interval_ms: u32) -> Config {
        Config {
            node_id,
            radio: RadioParams::EU868_DEFAULT,
            beacon_interval_ms,
            retry_delay_ms: 500,
        }
    }

    /// Adds a node and returns its index.
    pub fn add_node(&mut self, cfg: Config) -> usize {
        let id = self.nodes.len();
        self.medium.borrow_mut().radios.push(RadioState::default());
        let radio = SimRadio {
            id,
            medium: self.medium.clone(),
        };
        self.nodes
            .push(Node::new(radio, SimClock(self.clock.clone()), cfg));
        id
    }

    /// Adds a one-way link `from -> to`.
    pub fn link(&mut self, from: usize, to: usize, link: Link) {
        self.medium.borrow_mut().links.insert((from, to), link);
    }

    /// Adds the same link in both directions.
    pub fn link_both(&mut self, a: usize, b: usize, link: Link) {
        self.link(a, b, link);
        self.link(b, a, link);
    }

    /// Makes the next `times` radio initialisations of `node` fail.
    pub fn fail_begin(&mut self, node: usize, times: u32) {
        self.medium.borrow_mut().radios[node].fail_begin = times;
    }

    pub fn node(&self, index: usize) -> &SimNode {
        &self.nodes[index]
    }

    pub fn now(&self) -> u32 {
        self.clock.get()
    }

    /// Advances virtual time by `ms` milliseconds, one tick at a time.
    pub fn run_ms(&mut self, ms: u32) {
        for _ in 0..ms {
            self.clock.set(self.clock.get() + 1);
            self.medium.borrow_mut().advance();
            for node in &mut self.nodes {
                node.step();
            }
        }
    }
}
