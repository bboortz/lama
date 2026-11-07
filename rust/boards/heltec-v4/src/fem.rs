//! External front-end (PA/LNA) control.
//!
//! The V4 has an amplifier/LNA module between the SX1262 and the antenna.
//! Which module is fitted depends on the board revision, and the firmware
//! must drive its pins correctly or receive/transmit silently fails.
//!
//! | Revision | Module | Mode pin |
//! |---|---|---|
//! | V4.2 and earlier | GC1109 | GPIO46 |
//! | V4.3 | KCT8103L | GPIO5 |
//!
//! Both use GPIO2 as chip enable (CSD). The type can be detected at run time:
//! configure GPIO2 as a floating input shortly after power-up and read it.
//! The KCT8103L has an internal pull-up (reads high), the GC1109 an internal
//! pull-down (reads low). This method is taken from the MeshCore firmware; it
//! still needs confirming on real boards.
//!
//! The SX1262's DIO2 is also wired to the RF switch on these boards; the radio
//! driver should enable "DIO2 as RF switch" as Meshtastic does. How DIO2 and
//! the mode pin interact on each revision is to be confirmed on hardware.

use crate::pins;

/// The front-end module fitted to a board.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FrontEnd {
    Gc1109,
    Kct8103l,
}

/// What the front-end should be doing.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Mode {
    /// Power amplifier path.
    Tx,
    /// Receive through the LNA (the default).
    RxLna,
    /// Receive bypassing the LNA (KCT8103L only; the GC1109 has no separate bypass here).
    RxBypass,
    /// Front-end shut down.
    Sleep,
}

/// Output levels for the two control pins (`true` = high).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Levels {
    /// Chip enable (GPIO2).
    pub csd: bool,
    /// Mode pin ([`FrontEnd::ctrl_pin`]).
    pub ctrl: bool,
}

impl FrontEnd {
    /// Detects the front-end from the level read on GPIO2 (floating input,
    /// about 1 ms after the front-end is powered).
    pub const fn detect(csd_reads_high: bool) -> Self {
        if csd_reads_high {
            Self::Kct8103l
        } else {
            Self::Gc1109
        }
    }

    /// The GPIO that selects TX/RX on this front-end.
    pub const fn ctrl_pin(self) -> u8 {
        match self {
            Self::Gc1109 => pins::FEM_CTRL_GC1109,
            Self::Kct8103l => pins::FEM_CTRL_KCT8103L,
        }
    }

    /// The pin levels for a mode.
    pub const fn levels(self, mode: Mode) -> Levels {
        match (self, mode) {
            (_, Mode::Sleep) => Levels {
                csd: false,
                ctrl: false,
            },
            (_, Mode::Tx) => Levels {
                csd: true,
                ctrl: true,
            },
            (Self::Kct8103l, Mode::RxBypass) => Levels {
                csd: true,
                ctrl: true,
            },
            (_, Mode::RxLna | Mode::RxBypass) => Levels {
                csd: true,
                ctrl: false,
            },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn detection_follows_the_pull_resistor() {
        assert_eq!(FrontEnd::detect(true), FrontEnd::Kct8103l);
        assert_eq!(FrontEnd::detect(false), FrontEnd::Gc1109);
    }

    #[test]
    fn mode_pin_differs_per_front_end() {
        assert_eq!(FrontEnd::Gc1109.ctrl_pin(), 46);
        assert_eq!(FrontEnd::Kct8103l.ctrl_pin(), 5);
        assert_ne!(FrontEnd::Gc1109.ctrl_pin(), pins::FEM_CSD);
        assert_ne!(FrontEnd::Kct8103l.ctrl_pin(), pins::FEM_CSD);
    }

    #[test]
    fn transmit_enables_the_pa_on_both() {
        for fe in [FrontEnd::Gc1109, FrontEnd::Kct8103l] {
            assert_eq!(
                fe.levels(Mode::Tx),
                Levels {
                    csd: true,
                    ctrl: true
                }
            );
        }
    }

    #[test]
    fn receive_with_lna_is_csd_high_ctrl_low_on_both() {
        for fe in [FrontEnd::Gc1109, FrontEnd::Kct8103l] {
            assert_eq!(
                fe.levels(Mode::RxLna),
                Levels {
                    csd: true,
                    ctrl: false
                }
            );
        }
    }

    #[test]
    fn bypass_only_differs_on_the_kct8103l() {
        assert_eq!(
            FrontEnd::Kct8103l.levels(Mode::RxBypass),
            Levels {
                csd: true,
                ctrl: true
            }
        );
        assert_eq!(
            FrontEnd::Gc1109.levels(Mode::RxBypass),
            FrontEnd::Gc1109.levels(Mode::RxLna)
        );
    }

    #[test]
    fn sleep_shuts_the_front_end_down() {
        for fe in [FrontEnd::Gc1109, FrontEnd::Kct8103l] {
            assert_eq!(
                fe.levels(Mode::Sleep),
                Levels {
                    csd: false,
                    ctrl: false
                }
            );
        }
    }

    #[test]
    fn only_sleep_disables_the_chip() {
        for fe in [FrontEnd::Gc1109, FrontEnd::Kct8103l] {
            for m in [Mode::Tx, Mode::RxLna, Mode::RxBypass] {
                assert!(fe.levels(m).csd, "{fe:?} {m:?} must keep CSD high");
            }
        }
    }

    #[test]
    fn radio_pins_are_distinct() {
        let p = [
            pins::LORA_SCK,
            pins::LORA_MISO,
            pins::LORA_MOSI,
            pins::LORA_NSS,
            pins::LORA_RST,
            pins::LORA_BUSY,
            pins::LORA_DIO1,
            pins::FEM_POWER,
            pins::FEM_CSD,
            pins::FEM_CTRL_GC1109,
            pins::FEM_CTRL_KCT8103L,
        ];
        for (i, a) in p.iter().enumerate() {
            for b in &p[i + 1..] {
                assert_ne!(a, b, "pin {a} used twice");
            }
        }
    }
}
