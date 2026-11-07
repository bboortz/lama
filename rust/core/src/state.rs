//! The TX/RX state machine as pure logic: events in, actions out.
//!
//! The machine never touches hardware. The kernel/board layer feeds it
//! [`Event`]s (interrupts, timers, radio results) and carries out the returned
//! [`Action`]. That keeps it fully testable on a PC.
//!
//! ```text
//!   Init --Booted--> Ready --RadioReady--> Idle <--RxDone/TxDone-- Receiving / Transmitting
//!                      ^                    |  RxInterrupt -> Receiving
//!                      |                    |  TxDue       -> Transmitting
//!   Error --RetryElapsed                    |
//!   any (not Fatal) --Fault--> Error  (after MAX_ERRORS consecutive faults: Fatal)
//! ```

/// Consecutive faults (without a successful radio start in between) before the machine gives up.
pub const MAX_ERRORS: u8 = 3;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum State {
    /// Just powered on.
    Init,
    /// Radio is being initialised.
    Ready,
    /// Radio listening, nothing in progress.
    Idle,
    /// A packet is being read from the radio.
    Receiving,
    /// A packet is being transmitted.
    Transmitting,
    /// Recoverable fault, waiting to retry.
    Error,
    /// Unrecoverable: stop and wait for a reset.
    Fatal,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Event {
    /// Configuration loaded, system starting.
    Booted,
    /// Radio initialised successfully.
    RadioReady,
    /// Any radio or driver failure.
    Fault,
    /// Radio signalled a received packet.
    RxInterrupt,
    /// Packet has been read and handled.
    RxDone,
    /// The TX interval elapsed.
    TxDue,
    /// Transmission finished.
    TxDone,
    /// The retry delay after a fault elapsed.
    RetryElapsed,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    None,
    InitRadio,
    StartRx,
    ReadPacket,
    SendBeat,
    ScheduleRetry,
    Halt,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct StateMachine {
    state: State,
    /// A TX became due while a packet was being received; send right after.
    pending_tx: bool,
    errors: u8,
}

impl StateMachine {
    pub const fn new() -> Self {
        Self {
            state: State::Init,
            pending_tx: false,
            errors: 0,
        }
    }

    pub const fn state(&self) -> State {
        self.state
    }

    /// Consecutive faults seen since the radio last started successfully.
    pub const fn errors(&self) -> u8 {
        self.errors
    }

    /// Feeds one event and returns what the caller must do. Events that make
    /// no sense in the current state are ignored (`Action::None`).
    pub fn handle(&mut self, event: Event) -> Action {
        use Action as A;
        use Event as E;
        use State as S;

        if self.state == S::Fatal {
            return A::None;
        }

        if event == E::Fault {
            self.pending_tx = false;
            self.errors = self.errors.saturating_add(1);
            return if self.errors >= MAX_ERRORS {
                self.state = S::Fatal;
                A::Halt
            } else {
                self.state = S::Error;
                A::ScheduleRetry
            };
        }

        match (self.state, event) {
            (S::Init, E::Booted) => {
                self.state = S::Ready;
                A::InitRadio
            }
            (S::Ready, E::RadioReady) => {
                self.errors = 0;
                self.state = S::Idle;
                A::StartRx
            }
            (S::Error, E::RetryElapsed) => {
                self.state = S::Ready;
                A::InitRadio
            }
            (S::Idle, E::RxInterrupt) => {
                self.state = S::Receiving;
                A::ReadPacket
            }
            (S::Idle, E::TxDue) => {
                self.state = S::Transmitting;
                A::SendBeat
            }
            (S::Receiving, E::TxDue) => {
                self.pending_tx = true;
                A::None
            }
            (S::Receiving, E::RxDone) => {
                if self.pending_tx {
                    self.pending_tx = false;
                    self.state = S::Transmitting;
                    A::SendBeat
                } else {
                    self.state = S::Idle;
                    A::StartRx
                }
            }
            (S::Transmitting, E::TxDone) => {
                self.state = S::Idle;
                A::StartRx
            }
            _ => A::None,
        }
    }
}

impl Default for StateMachine {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use Action as A;
    use Event as E;
    use State as S;

    fn idle() -> StateMachine {
        let mut m = StateMachine::new();
        assert_eq!(m.handle(E::Booted), A::InitRadio);
        assert_eq!(m.handle(E::RadioReady), A::StartRx);
        assert_eq!(m.state(), S::Idle);
        m
    }

    #[test]
    fn boot_sequence() {
        let mut m = StateMachine::new();
        assert_eq!(m.state(), S::Init);
        assert_eq!(m.handle(E::Booted), A::InitRadio);
        assert_eq!(m.state(), S::Ready);
        assert_eq!(m.handle(E::RadioReady), A::StartRx);
        assert_eq!(m.state(), S::Idle);
    }

    #[test]
    fn receive_cycle() {
        let mut m = idle();
        assert_eq!(m.handle(E::RxInterrupt), A::ReadPacket);
        assert_eq!(m.state(), S::Receiving);
        assert_eq!(m.handle(E::RxDone), A::StartRx);
        assert_eq!(m.state(), S::Idle);
    }

    #[test]
    fn transmit_cycle() {
        let mut m = idle();
        assert_eq!(m.handle(E::TxDue), A::SendBeat);
        assert_eq!(m.state(), S::Transmitting);
        assert_eq!(m.handle(E::TxDone), A::StartRx);
        assert_eq!(m.state(), S::Idle);
    }

    #[test]
    fn tx_due_during_rx_is_deferred_not_lost() {
        let mut m = idle();
        m.handle(E::RxInterrupt);
        assert_eq!(m.handle(E::TxDue), A::None);
        assert_eq!(m.state(), S::Receiving);
        assert_eq!(m.handle(E::RxDone), A::SendBeat);
        assert_eq!(m.state(), S::Transmitting);
        assert_eq!(m.handle(E::TxDone), A::StartRx);
        // and only once
        m.handle(E::RxInterrupt);
        assert_eq!(m.handle(E::RxDone), A::StartRx);
    }

    #[test]
    fn rx_interrupt_while_transmitting_is_ignored() {
        let mut m = idle();
        m.handle(E::TxDue);
        assert_eq!(m.handle(E::RxInterrupt), A::None);
        assert_eq!(m.state(), S::Transmitting);
    }

    #[test]
    fn fault_then_retry_recovers_and_resets_error_count() {
        let mut m = idle();
        assert_eq!(m.handle(E::Fault), A::ScheduleRetry);
        assert_eq!(m.state(), S::Error);
        assert_eq!(m.errors(), 1);
        assert_eq!(m.handle(E::RetryElapsed), A::InitRadio);
        assert_eq!(m.state(), S::Ready);
        assert_eq!(m.handle(E::RadioReady), A::StartRx);
        assert_eq!(m.errors(), 0);
    }

    #[test]
    fn repeated_init_failure_ends_in_fatal() {
        let mut m = StateMachine::new();
        m.handle(E::Booted);
        let mut last = A::None;
        for _ in 0..MAX_ERRORS {
            last = m.handle(E::Fault);
            if m.state() == S::Error {
                m.handle(E::RetryElapsed);
            }
        }
        assert_eq!(last, A::Halt);
        assert_eq!(m.state(), S::Fatal);
    }

    #[test]
    fn fatal_absorbs_everything() {
        let mut m = StateMachine::new();
        for _ in 0..MAX_ERRORS {
            m.handle(E::Fault);
        }
        assert_eq!(m.state(), S::Fatal);
        for e in [
            E::Booted,
            E::RadioReady,
            E::RxInterrupt,
            E::RxDone,
            E::TxDue,
            E::TxDone,
            E::RetryElapsed,
            E::Fault,
        ] {
            assert_eq!(m.handle(e), A::None);
            assert_eq!(m.state(), S::Fatal);
        }
    }

    #[test]
    fn fault_clears_pending_tx() {
        let mut m = idle();
        m.handle(E::RxInterrupt);
        m.handle(E::TxDue); // deferred
        m.handle(E::Fault);
        m.handle(E::RetryElapsed);
        m.handle(E::RadioReady);
        m.handle(E::RxInterrupt);
        assert_eq!(m.handle(E::RxDone), A::StartRx); // no stale beat
    }

    #[test]
    fn nonsensical_events_leave_state_unchanged() {
        let mut m = StateMachine::new();
        for e in [
            E::RadioReady,
            E::RxInterrupt,
            E::RxDone,
            E::TxDue,
            E::TxDone,
            E::RetryElapsed,
        ] {
            assert_eq!(m.handle(e), A::None);
            assert_eq!(m.state(), S::Init);
        }
        let mut m = idle();
        for e in [
            E::Booted,
            E::RadioReady,
            E::RxDone,
            E::TxDone,
            E::RetryElapsed,
        ] {
            assert_eq!(m.handle(e), A::None);
            assert_eq!(m.state(), S::Idle);
        }
    }

    #[test]
    fn tx_due_is_dropped_outside_idle_and_receiving() {
        let mut m = StateMachine::new();
        assert_eq!(m.handle(E::TxDue), A::None);
        m.handle(E::Booted);
        assert_eq!(m.handle(E::TxDue), A::None);
        assert_eq!(m.state(), S::Ready);
    }
}
