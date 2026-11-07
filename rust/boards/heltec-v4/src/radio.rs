//! SX1262 radio driver for the Heltec V4, built on `lora-phy`.
//!
//! Generic over `embedded-hal(-async)` traits, so it contains no chip-specific
//! code: the esp-hal glue only has to hand it an SPI device, a delay and the pins.
//! [`HeltecDriver`] implements [`lama_radio_bridge::RadioDriver`].
//!
//! Board specifics handled here:
//! * 1.8 V TCXO supplied from DIO3, DIO2 drives the RF switch, DC-DC enabled;
//! * the external front-end: powered through `fem_power`, and its CSD / mode
//!   pins follow the radio state through [`crate::fem`];
//! * a transmit power cap, because the front-end amplifies the SX1262's output.

use embedded_hal::digital::OutputPin;
use embedded_hal_async::delay::DelayNs;
use embedded_hal_async::digital::Wait;
use embedded_hal_async::spi::SpiDevice;
use lama_hal::{RadioParams, RxMeta};
use lama_radio_bridge::RadioDriver;
use lora_phy::mod_params::{
    Bandwidth, CodingRate, ModulationParams, PacketParams, RadioError, SpreadingFactor,
};
use lora_phy::mod_traits::InterfaceVariant;
use lora_phy::sx126x::{Config as SxConfig, Sx1262, Sx126x, TcxoCtrlVoltage};
use lora_phy::{LoRa, RxMode};

use crate::fem::{FrontEnd, Mode};

/// Highest power the driver asks the SX1262 for. The front-end adds gain on
/// top (up to roughly 28 dBm at the antenna connector at full drive), and EU868
/// limits are far lower. This is a conservative cap until the output has been
/// measured on a real board; raise it deliberately.
pub const MAX_CHIP_TX_DBM: i8 = 10;

/// Radio parameters that cannot be mapped to the chip.
#[derive(Debug, PartialEq, Eq)]
pub enum ParamError {
    SpreadingFactor,
    Bandwidth,
    CodingRate,
}

/// Maps LAMA radio parameters to `lora-phy` types.
pub fn map_params(p: &RadioParams) -> Result<(SpreadingFactor, Bandwidth, CodingRate), ParamError> {
    let sf = match p.spreading_factor {
        5 => SpreadingFactor::_5,
        6 => SpreadingFactor::_6,
        7 => SpreadingFactor::_7,
        8 => SpreadingFactor::_8,
        9 => SpreadingFactor::_9,
        10 => SpreadingFactor::_10,
        11 => SpreadingFactor::_11,
        12 => SpreadingFactor::_12,
        _ => return Err(ParamError::SpreadingFactor),
    };
    let bw = match p.bandwidth_hz {
        62_500 => Bandwidth::_62KHz,
        125_000 => Bandwidth::_125KHz,
        250_000 => Bandwidth::_250KHz,
        500_000 => Bandwidth::_500KHz,
        _ => return Err(ParamError::Bandwidth),
    };
    let cr = match p.coding_rate {
        5 => CodingRate::_4_5,
        6 => CodingRate::_4_6,
        7 => CodingRate::_4_7,
        8 => CodingRate::_4_8,
        _ => return Err(ParamError::CodingRate),
    };
    Ok((sf, bw, cr))
}

/// Power actually requested from the chip.
pub fn chip_tx_power(requested_dbm: i8) -> i32 {
    i32::from(requested_dbm.min(MAX_CHIP_TX_DBM))
}

/// `true` selects the public LoRa sync word (0x34), anything else the private one (0x12).
pub fn is_public_network(sync_word: u8) -> bool {
    sync_word == 0x34
}

/// Interface between `lora-phy` and the V4's control pins.
///
/// `CTRL` are output pins, `WAIT` input pins with async edge/level waiting.
pub struct HeltecIv<CTRL, WAIT> {
    reset: CTRL,
    busy: WAIT,
    dio1: WAIT,
    fem_csd: CTRL,
    fem_ctrl: CTRL,
    front_end: FrontEnd,
}

impl<CTRL: OutputPin, WAIT: Wait> HeltecIv<CTRL, WAIT> {
    /// Powers the front-end (`fem_power` high) and puts it to sleep.
    ///
    /// `fem_ctrl` must be the pin for `front_end` ([`FrontEnd::ctrl_pin`]).
    pub fn new(
        reset: CTRL,
        busy: WAIT,
        dio1: WAIT,
        mut fem_power: CTRL,
        fem_csd: CTRL,
        fem_ctrl: CTRL,
        front_end: FrontEnd,
    ) -> Result<Self, RadioError> {
        fem_power.set_high().map_err(|_| RadioError::RfSwitchRx)?;
        let mut iv = Self {
            reset,
            busy,
            dio1,
            fem_csd,
            fem_ctrl,
            front_end,
        };
        iv.set_fem(Mode::Sleep, || RadioError::RfSwitchRx)?;
        Ok(iv)
    }

    fn set_fem(&mut self, mode: Mode, err: fn() -> RadioError) -> Result<(), RadioError> {
        let lv = self.front_end.levels(mode);
        // Order: enable chip before steering, disable chip last.
        if lv.csd {
            self.fem_csd.set_high().map_err(|_| err())?;
        }
        if lv.ctrl {
            self.fem_ctrl.set_high()
        } else {
            self.fem_ctrl.set_low()
        }
        .map_err(|_| err())?;
        if !lv.csd {
            self.fem_csd.set_low().map_err(|_| err())?;
        }
        Ok(())
    }
}

impl<CTRL: OutputPin, WAIT: Wait> InterfaceVariant for HeltecIv<CTRL, WAIT> {
    async fn reset(&mut self, delay: &mut impl DelayNs) -> Result<(), RadioError> {
        delay.delay_ms(10).await;
        self.reset.set_low().map_err(|_| RadioError::Reset)?;
        delay.delay_ms(20).await;
        self.reset.set_high().map_err(|_| RadioError::Reset)?;
        delay.delay_ms(10).await;
        Ok(())
    }

    async fn wait_on_busy(&mut self) -> Result<(), RadioError> {
        self.busy.wait_for_low().await.map_err(|_| RadioError::Busy)
    }

    async fn await_irq(&mut self) -> Result<(), RadioError> {
        self.dio1
            .wait_for_high()
            .await
            .map_err(|_| RadioError::DIO1)
    }

    async fn enable_rf_switch_rx(&mut self) -> Result<(), RadioError> {
        self.set_fem(Mode::RxLna, || RadioError::RfSwitchRx)
    }

    async fn enable_rf_switch_tx(&mut self) -> Result<(), RadioError> {
        self.set_fem(Mode::Tx, || RadioError::RfSwitchTx)
    }

    async fn disable_rf_switch(&mut self) -> Result<(), RadioError> {
        self.set_fem(Mode::Sleep, || RadioError::RfSwitchRx)
    }
}

/// Everything needed after the first successful `init`.
struct Active {
    modulation: ModulationParams,
    tx_packet: PacketParams,
    rx_packet: PacketParams,
    tx_power: i32,
}

type Lora<SPI, IV, DLY> = LoRa<Sx126x<SPI, IV, Sx1262>, DLY>;

/// SX1262 on the Heltec V4 as a [`RadioDriver`].
pub struct HeltecDriver<SPI, IV, DLY>
where
    SPI: SpiDevice<u8>,
    IV: InterfaceVariant,
    DLY: DelayNs,
{
    parts: Option<(SPI, IV, DLY)>,
    lora: Option<Lora<SPI, IV, DLY>>,
    active: Option<Active>,
}

/// Driver errors.
#[derive(Debug)]
pub enum DriverError {
    Params(ParamError),
    Radio(RadioError),
    /// `transmit`/`receive` before a successful `init`.
    NotInitialised,
}

impl From<RadioError> for DriverError {
    fn from(e: RadioError) -> Self {
        Self::Radio(e)
    }
}

impl<SPI, IV, DLY> HeltecDriver<SPI, IV, DLY>
where
    SPI: SpiDevice<u8>,
    IV: InterfaceVariant,
    DLY: DelayNs,
{
    pub fn new(spi: SPI, iv: IV, delay: DLY) -> Self {
        Self {
            parts: Some((spi, iv, delay)),
            lora: None,
            active: None,
        }
    }
}

impl<SPI, IV, DLY> RadioDriver for HeltecDriver<SPI, IV, DLY>
where
    SPI: SpiDevice<u8>,
    IV: InterfaceVariant,
    DLY: DelayNs,
{
    type Error = DriverError;

    async fn init(&mut self, p: &RadioParams) -> Result<(), DriverError> {
        self.active = None;
        let (sf, bw, cr) = map_params(p).map_err(DriverError::Params)?;

        if self.lora.is_none() {
            let (spi, iv, delay) = self.parts.take().ok_or(DriverError::NotInitialised)?;
            let cfg = SxConfig {
                chip: Sx1262,
                tcxo_ctrl: Some(TcxoCtrlVoltage::Ctrl1V8),
                use_dcdc: true,
                rx_boost: false,
            };
            let kind = Sx126x::new(spi, iv, cfg);
            self.lora = Some(LoRa::new(kind, is_public_network(p.sync_word), delay).await?);
        } else if let Some(l) = self.lora.as_mut() {
            l.init().await?; // retry path: reset and reconfigure
        }

        let lora = self.lora.as_mut().ok_or(DriverError::NotInitialised)?;
        let modulation = lora.create_modulation_params(sf, bw, cr, p.freq_hz)?;
        let tx_packet =
            lora.create_tx_packet_params(p.preamble_len, false, true, false, &modulation)?;
        let rx_packet = lora.create_rx_packet_params(
            p.preamble_len,
            false,
            lama_radio_bridge::MAX_FRAME as u8,
            true,
            false,
            &modulation,
        )?;
        self.active = Some(Active {
            modulation,
            tx_packet,
            rx_packet,
            tx_power: chip_tx_power(p.tx_power_dbm),
        });
        Ok(())
    }

    async fn transmit(&mut self, frame: &[u8]) -> Result<(), DriverError> {
        let (lora, a) = self
            .lora
            .as_mut()
            .zip(self.active.as_mut())
            .ok_or(DriverError::NotInitialised)?;
        lora.prepare_for_tx(&a.modulation, &mut a.tx_packet, a.tx_power, frame)
            .await?;
        lora.tx().await?;
        Ok(())
    }

    async fn receive(&mut self, buf: &mut [u8]) -> Result<(usize, RxMeta), DriverError> {
        let (lora, a) = self
            .lora
            .as_mut()
            .zip(self.active.as_ref())
            .ok_or(DriverError::NotInitialised)?;
        lora.prepare_for_rx(RxMode::Continuous, &a.modulation, &a.rx_packet)
            .await?;
        let (len, status) = lora.rx(&a.rx_packet, buf).await?;
        Ok((
            usize::from(len),
            RxMeta {
                rssi_cdbm: status.rssi.saturating_mul(100),
                snr_cdb: status.snr.saturating_mul(100),
            },
        ))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_params_map() {
        let (sf, bw, cr) = map_params(&RadioParams::EU868_DEFAULT).unwrap();
        assert!(matches!(sf, SpreadingFactor::_7));
        assert!(matches!(bw, Bandwidth::_125KHz));
        assert!(matches!(cr, CodingRate::_4_5));
    }

    #[test]
    fn bad_params_are_rejected() {
        let mut p = RadioParams::EU868_DEFAULT;
        p.spreading_factor = 13;
        assert_eq!(map_params(&p).unwrap_err(), ParamError::SpreadingFactor);
        let mut p = RadioParams::EU868_DEFAULT;
        p.bandwidth_hz = 100_000;
        assert_eq!(map_params(&p).unwrap_err(), ParamError::Bandwidth);
        let mut p = RadioParams::EU868_DEFAULT;
        p.coding_rate = 9;
        assert_eq!(map_params(&p).unwrap_err(), ParamError::CodingRate);
    }

    #[test]
    fn tx_power_is_capped() {
        assert_eq!(chip_tx_power(22), i32::from(MAX_CHIP_TX_DBM));
        assert_eq!(chip_tx_power(5), 5);
        assert_eq!(chip_tx_power(-3), -3);
    }

    #[test]
    fn sync_word_selects_network() {
        assert!(is_public_network(0x34));
        assert!(!is_public_network(0x12));
    }

    // --- front-end wiring, with mock pins ---

    use core::cell::Cell;
    use core::convert::Infallible;
    use embassy_futures::block_on;
    use embedded_hal::digital::ErrorType;

    #[derive(Clone, Copy)]
    struct Pin<'a>(&'a Cell<bool>);
    impl ErrorType for Pin<'_> {
        type Error = Infallible;
    }
    impl OutputPin for Pin<'_> {
        fn set_low(&mut self) -> Result<(), Infallible> {
            self.0.set(false);
            Ok(())
        }
        fn set_high(&mut self) -> Result<(), Infallible> {
            self.0.set(true);
            Ok(())
        }
    }
    impl embedded_hal_async::digital::Wait for Pin<'_> {
        async fn wait_for_high(&mut self) -> Result<(), Infallible> {
            Ok(())
        }
        async fn wait_for_low(&mut self) -> Result<(), Infallible> {
            Ok(())
        }
        async fn wait_for_rising_edge(&mut self) -> Result<(), Infallible> {
            Ok(())
        }
        async fn wait_for_falling_edge(&mut self) -> Result<(), Infallible> {
            Ok(())
        }
        async fn wait_for_any_edge(&mut self) -> Result<(), Infallible> {
            Ok(())
        }
    }

    #[test]
    fn front_end_pins_follow_radio_state_on_both_modules() {
        for fe in [FrontEnd::Gc1109, FrontEnd::Kct8103l] {
            let (rst, busy, dio1) = (Cell::new(true), Cell::new(false), Cell::new(false));
            let (power, csd, ctrl) = (Cell::new(false), Cell::new(true), Cell::new(true));
            let mut iv = HeltecIv::new(
                Pin(&rst),
                Pin(&busy),
                Pin(&dio1),
                Pin(&power),
                Pin(&csd),
                Pin(&ctrl),
                fe,
            )
            .unwrap();
            assert!(power.get(), "front-end must be powered");
            assert!(!csd.get() && !ctrl.get(), "starts asleep");

            block_on(iv.enable_rf_switch_tx()).unwrap();
            assert!(csd.get() && ctrl.get(), "{fe:?} tx");
            block_on(iv.enable_rf_switch_rx()).unwrap();
            assert!(csd.get() && !ctrl.get(), "{fe:?} rx");
            block_on(iv.disable_rf_switch()).unwrap();
            assert!(!csd.get() && !ctrl.get(), "{fe:?} sleep");
        }
    }
}
