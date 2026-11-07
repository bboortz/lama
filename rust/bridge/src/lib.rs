//! Bridge between an async radio driver and the synchronous [`Radio`] trait.
//!
//! Real radio drivers (`lora-phy`) are `async`; the node logic in `lama-app` is
//! a plain non-blocking loop. The bridge connects the two without a heap:
//!
//! ```text
//!   Node ── BridgeRadio ──(commands, events, rx frames)── Shared ── run() ── RadioDriver
//!   sync, never blocks                static, fixed-size                   async task
//! ```
//!
//! * [`BridgeRadio`] implements [`Radio`]: calls only store a command and wake the task.
//! * [`run`] is the task: it executes commands on the [`RadioDriver`], listens
//!   for frames while receiving, and reports results as [`RadioEvent`]s.
//!
//! Only one command is outstanding at a time (the node's state machine already
//! guarantees this); a second one is answered with [`BridgeError::Busy`].
//!
//! Receive is cancelled when a new command arrives, so
//! [`RadioDriver::receive`] must tolerate being dropped mid-way. After a
//! transmission the radio is not listening until the node calls `start_rx`.

#![no_std]
#![forbid(unsafe_code)]

use core::cell::RefCell;

use embassy_futures::select::{select, Either};
use embassy_sync::blocking_mutex::raw::CriticalSectionRawMutex;
use embassy_sync::blocking_mutex::Mutex;
use embassy_sync::signal::Signal;
use lama_hal::{Radio, RadioEvent, RadioParams, RxMeta};

/// Longest frame the bridge carries (matches the LAMA wire format).
pub const MAX_FRAME: usize = 39;
const EVENT_CAP: usize = 8;
const RX_CAP: usize = 4;

/// What the async side must provide.
#[allow(async_fn_in_trait)]
pub trait RadioDriver {
    type Error: core::fmt::Debug;

    /// Resets and configures the radio.
    async fn init(&mut self, params: &RadioParams) -> Result<(), Self::Error>;
    /// Sends one frame and returns when it is on the air.
    async fn transmit(&mut self, frame: &[u8]) -> Result<(), Self::Error>;
    /// Waits for one frame; returns its length and link quality.
    async fn receive(&mut self, buf: &mut [u8]) -> Result<(usize, RxMeta), Self::Error>;
}

#[derive(Debug, PartialEq, Eq)]
pub enum BridgeError {
    /// A command is already waiting for the task.
    Busy,
    TooLong,
}

#[derive(Clone, Copy)]
enum Command {
    Begin(RadioParams),
    StartRx,
    Transmit { len: usize, data: [u8; MAX_FRAME] },
}

/// Small fixed-capacity FIFO; when full the newest item is dropped.
struct Queue<T: Copy, const N: usize> {
    items: [Option<T>; N],
    head: usize,
    len: usize,
}

impl<T: Copy, const N: usize> Queue<T, N> {
    const fn new() -> Self {
        Self {
            items: [None; N],
            head: 0,
            len: 0,
        }
    }
    fn push(&mut self, item: T) -> bool {
        if self.len == N {
            return false;
        }
        self.items[(self.head + self.len) % N] = Some(item);
        self.len += 1;
        true
    }
    fn pop(&mut self) -> Option<T> {
        if self.len == 0 {
            return None;
        }
        let item = self.items[self.head].take();
        self.head = (self.head + 1) % N;
        self.len -= 1;
        item
    }
}

#[derive(Clone, Copy)]
struct RxFrame {
    len: usize,
    data: [u8; MAX_FRAME],
    meta: RxMeta,
}

struct Inner {
    command: Option<Command>,
    receiving: bool,
    events: Queue<RadioEvent, EVENT_CAP>,
    rx: Queue<RxFrame, RX_CAP>,
    /// Events or frames dropped because a queue was full.
    overflow: u32,
}

/// State shared between [`BridgeRadio`] and [`run`]. Put it in a `static`.
pub struct Shared {
    inner: Mutex<CriticalSectionRawMutex, RefCell<Inner>>,
    wake: Signal<CriticalSectionRawMutex, ()>,
}

impl Default for Shared {
    fn default() -> Self {
        Self::new()
    }
}

impl Shared {
    pub const fn new() -> Self {
        Self {
            inner: Mutex::new(RefCell::new(Inner {
                command: None,
                receiving: false,
                events: Queue::new(),
                rx: Queue::new(),
                overflow: 0,
            })),
            wake: Signal::new(),
        }
    }

    fn with<R>(&self, f: impl FnOnce(&mut Inner) -> R) -> R {
        self.inner.lock(|c| f(&mut c.borrow_mut()))
    }

    /// Events or frames lost to full queues (should stay 0).
    pub fn overflow_count(&self) -> u32 {
        self.with(|i| i.overflow)
    }
}

fn post(inner: &mut Inner, ev: RadioEvent) {
    if !inner.events.push(ev) {
        inner.overflow = inner.overflow.saturating_add(1);
    }
}

/// The synchronous face of the bridge; hand this to the `Node`.
pub struct BridgeRadio<'a> {
    shared: &'a Shared,
}

impl<'a> BridgeRadio<'a> {
    pub fn new(shared: &'a Shared) -> Self {
        Self { shared }
    }

    fn submit(&self, cmd: Command) -> Result<(), BridgeError> {
        self.shared.with(|i| {
            if i.command.is_some() {
                return Err(BridgeError::Busy);
            }
            i.command = Some(cmd);
            Ok(())
        })?;
        self.shared.wake.signal(());
        Ok(())
    }
}

impl Radio for BridgeRadio<'_> {
    type Error = BridgeError;

    fn begin(&mut self, params: &RadioParams) -> Result<(), BridgeError> {
        self.submit(Command::Begin(*params))
    }

    fn start_rx(&mut self) -> Result<(), BridgeError> {
        self.submit(Command::StartRx)
    }

    fn transmit(&mut self, frame: &[u8]) -> Result<(), BridgeError> {
        if frame.len() > MAX_FRAME {
            return Err(BridgeError::TooLong);
        }
        let mut data = [0u8; MAX_FRAME];
        data[..frame.len()].copy_from_slice(frame);
        self.submit(Command::Transmit {
            len: frame.len(),
            data,
        })
    }

    fn poll_event(&mut self) -> Option<RadioEvent> {
        self.shared.with(|i| i.events.pop())
    }

    fn read(&mut self, buf: &mut [u8]) -> Result<Option<(usize, RxMeta)>, BridgeError> {
        Ok(self.shared.with(|i| i.rx.pop()).map(|f| {
            let n = f.len.min(buf.len());
            buf[..n].copy_from_slice(&f.data[..n]);
            (n, f.meta)
        }))
    }
}

/// The driver task. Spawn it once; it never returns.
pub async fn run<D: RadioDriver>(driver: &mut D, shared: &Shared) -> ! {
    let mut buf = [0u8; MAX_FRAME];
    loop {
        if let Some(cmd) = shared.with(|i| i.command.take()) {
            execute(driver, shared, cmd).await;
            continue;
        }
        if shared.with(|i| i.receiving) {
            match select(shared.wake.wait(), driver.receive(&mut buf)).await {
                Either::First(()) => {} // new command: loop around and run it
                Either::Second(Ok((len, meta))) => shared.with(|i| {
                    let mut data = [0u8; MAX_FRAME];
                    let n = len.min(MAX_FRAME);
                    data[..n].copy_from_slice(&buf[..n]);
                    if i.rx.push(RxFrame { len: n, data, meta }) {
                        post(i, RadioEvent::RxReady);
                    } else {
                        i.overflow = i.overflow.saturating_add(1);
                    }
                }),
                Either::Second(Err(_)) => shared.with(|i| {
                    i.receiving = false;
                    post(i, RadioEvent::Error);
                }),
            }
        } else {
            shared.wake.wait().await;
        }
    }
}

async fn execute<D: RadioDriver>(driver: &mut D, shared: &Shared, cmd: Command) {
    match cmd {
        Command::Begin(params) => {
            shared.with(|i| i.receiving = false);
            let ev = match driver.init(&params).await {
                Ok(()) => RadioEvent::Ready,
                Err(_) => RadioEvent::Error,
            };
            shared.with(|i| post(i, ev));
        }
        Command::StartRx => shared.with(|i| i.receiving = true),
        Command::Transmit { len, data } => {
            shared.with(|i| i.receiving = false); // half duplex
            let ev = match driver.transmit(&data[..len]).await {
                Ok(()) => RadioEvent::TxDone,
                Err(_) => RadioEvent::Error,
            };
            shared.with(|i| post(i, ev));
        }
    }
}
