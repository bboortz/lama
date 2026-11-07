//! Host tests for the bridge: a mock async driver stands in for lora-phy.

use std::cell::{Cell, RefCell};
use std::collections::VecDeque;
use std::future::Future;
use std::pin::{pin, Pin};
use std::rc::Rc;
use std::task::{Context, Poll};

use embassy_futures::poll_once;
use lama_app::{Config, Node};
use lama_core::state::State;
use lama_hal::{Clock, Radio, RadioEvent, RadioParams, RxMeta};
use lama_radio_bridge::{run, BridgeRadio, RadioDriver, Shared};

const META: RxMeta = RxMeta {
    rssi_cdbm: -9000,
    snr_cdb: 700,
};

/// Frames on the air: one inbox per mock radio.
type Air = Rc<RefCell<Vec<VecDeque<Vec<u8>>>>>;

struct MockDriver {
    id: usize,
    air: Air,
    fail_init: bool,
    inits: u32,
}

/// Pending until the inbox has a frame (repolled every tick by the test).
struct WaitFrame<'a>(&'a MockDriver);

impl Future for WaitFrame<'_> {
    type Output = Vec<u8>;
    fn poll(self: Pin<&mut Self>, _: &mut Context<'_>) -> Poll<Vec<u8>> {
        match self.0.air.borrow_mut()[self.0.id].pop_front() {
            Some(f) => Poll::Ready(f),
            None => Poll::Pending,
        }
    }
}

impl RadioDriver for MockDriver {
    type Error = ();

    async fn init(&mut self, _: &RadioParams) -> Result<(), ()> {
        self.inits += 1;
        if self.fail_init {
            self.fail_init = false;
            Err(())
        } else {
            Ok(())
        }
    }

    async fn transmit(&mut self, frame: &[u8]) -> Result<(), ()> {
        let mut air = self.air.borrow_mut();
        for (i, inbox) in air.iter_mut().enumerate() {
            if i != self.id {
                inbox.push_back(frame.to_vec());
            }
        }
        Ok(())
    }

    async fn receive(&mut self, buf: &mut [u8]) -> Result<(usize, RxMeta), ()> {
        let f = WaitFrame(self).await;
        buf[..f.len()].copy_from_slice(&f);
        Ok((f.len(), META))
    }
}

#[derive(Clone)]
struct TestClock(Rc<Cell<u32>>);
impl Clock for TestClock {
    fn now_ms(&self) -> u32 {
        self.0.get()
    }
}

fn cfg(id: u8, interval: u32) -> Config {
    Config {
        node_id: id,
        radio: RadioParams::EU868_DEFAULT,
        beacon_interval_ms: interval,
        retry_delay_ms: 500,
        op_timeout_ms: 2000,
    }
}

fn air(n: usize) -> Air {
    Rc::new(RefCell::new(vec![VecDeque::new(); n]))
}

#[test]
fn commands_become_events() {
    let shared = Shared::new();
    let a = air(1);
    let mut drv = MockDriver {
        id: 0,
        air: a,
        fail_init: false,
        inits: 0,
    };
    let mut radio = BridgeRadio::new(&shared);
    let mut task = pin!(run(&mut drv, &shared));

    assert!(radio.poll_event().is_none());
    radio.begin(&RadioParams::EU868_DEFAULT).unwrap();
    // a second command while one is queued is refused
    assert!(radio.start_rx().is_err());
    let _ = poll_once(task.as_mut());
    assert_eq!(radio.poll_event(), Some(RadioEvent::Ready));

    radio.transmit(&[1, 2, 3]).unwrap();
    let _ = poll_once(task.as_mut());
    assert_eq!(radio.poll_event(), Some(RadioEvent::TxDone));
    assert_eq!(shared.overflow_count(), 0);
}

#[test]
fn init_failure_is_reported_as_error_event() {
    let shared = Shared::new();
    let mut drv = MockDriver {
        id: 0,
        air: air(1),
        fail_init: true,
        inits: 0,
    };
    let mut radio = BridgeRadio::new(&shared);
    let mut task = pin!(run(&mut drv, &shared));
    radio.begin(&RadioParams::EU868_DEFAULT).unwrap();
    let _ = poll_once(task.as_mut());
    assert_eq!(radio.poll_event(), Some(RadioEvent::Error));
}

#[test]
fn oversized_frame_is_rejected() {
    let shared = Shared::new();
    let mut radio = BridgeRadio::new(&shared);
    assert!(radio.transmit(&[0u8; 40]).is_err());
}

#[test]
fn received_frame_is_read_back_with_meta() {
    let shared = Shared::new();
    let a = air(2);
    let mut drv = MockDriver {
        id: 0,
        air: a.clone(),
        fail_init: false,
        inits: 0,
    };
    let mut radio = BridgeRadio::new(&shared);
    let mut task = pin!(run(&mut drv, &shared));
    radio.begin(&RadioParams::EU868_DEFAULT).unwrap();
    let _ = poll_once(task.as_mut());
    radio.poll_event();
    radio.start_rx().unwrap();
    let _ = poll_once(task.as_mut());

    a.borrow_mut()[0].push_back(vec![9, 8, 7]);
    let _ = poll_once(task.as_mut());
    assert_eq!(radio.poll_event(), Some(RadioEvent::RxReady));
    let mut buf = [0u8; 39];
    let (n, meta) = radio.read(&mut buf).unwrap().unwrap();
    assert_eq!(&buf[..n], &[9, 8, 7]);
    assert_eq!(meta, META);
    assert!(radio.read(&mut buf).unwrap().is_none());
}

#[test]
fn two_real_nodes_talk_over_bridges() {
    let clock = Rc::new(Cell::new(0u32));
    let a = air(2);
    let (sh0, sh1) = (Shared::new(), Shared::new());
    let mut d0 = MockDriver {
        id: 0,
        air: a.clone(),
        fail_init: true, // node 1 has to retry its first init
        inits: 0,
    };
    let mut d1 = MockDriver {
        id: 1,
        air: a,
        fail_init: false,
        inits: 0,
    };
    let mut t0 = pin!(run(&mut d0, &sh0));
    let mut t1 = pin!(run(&mut d1, &sh1));
    let mut n0: Node<_, _, 4> = Node::new(
        BridgeRadio::new(&sh0),
        TestClock(clock.clone()),
        cfg(1, 1000),
    );
    let mut n1: Node<_, _, 4> = Node::new(
        BridgeRadio::new(&sh1),
        TestClock(clock.clone()),
        cfg(2, 1300),
    );

    for _ in 0..10_000 {
        clock.set(clock.get() + 1);
        let _ = poll_once(t0.as_mut());
        let _ = poll_once(t1.as_mut());
        n0.step();
        n1.step();
        let _ = poll_once(t0.as_mut());
        let _ = poll_once(t1.as_mut());
    }

    assert_eq!(n0.state(), State::Idle);
    assert_eq!(n1.state(), State::Idle);
    let seen_by_1 = n1.table().get(1).expect("n1 hears n0");
    let seen_by_0 = n0.table().get(2).expect("n0 hears n1");
    assert!(seen_by_1.rx_count >= 7, "{}", seen_by_1.rx_count);
    assert!(seen_by_0.rx_count >= 6, "{}", seen_by_0.rx_count);
    assert_eq!(seen_by_1.lost_count, 0);
    assert_eq!(seen_by_1.rssi.last, META.rssi_cdbm);
    assert_eq!(sh0.overflow_count() + sh1.overflow_count(), 0);
}
