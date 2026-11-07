# LAMA Rust workspace

Work in progress, see [`docs/design/0001-layered-core.md`](../docs/design/0001-layered-core.md).
The C++ firmware in the repository root stays the working reference until the
Rust version reaches parity.

## Crates

| Crate | What | Status |
|---|---|---|
| `core` (`lama-core`) | `no_std`, dependency-free: wire format v2 (`packet`), link statistics (`stats`), TX/RX state machine (`state`) | implemented, 33 tests |
| `hal` (`lama-hal`) | `no_std` traits: `Radio` (non-blocking, event polling), `Clock`, `RadioParams` | implemented |
| `app` (`lama-app`) | `no_std` `Node`: runs the state machine against a `Radio` + `Clock`, sends beacons, records link stats, retries radio init | implemented |
| `sim` (`lama-sim`) | Deterministic network simulator: virtual clock, airtime, half-duplex, collisions, per-link loss and RSSI/SNR, fault injection | implemented, 13 tests |
| `bridge` (`lama-radio-bridge`) | `no_std`, no heap: adapts an async radio driver to the sync `Radio` trait (command slot, event and frame queues, driver task) | implemented, 5 tests |
| `boards/heltec-v4` | Heltec WiFi LoRa 32 V4: pin map, front-end (GC1109 / KCT8103L) logic, and with feature `lora` an SX1262 driver on `lora-phy` | driver compiles for Cortex-M4 and RISC-V, **not yet built for ESP32-S3 or run on hardware**; 13 tests |

Planned next: Heltec firmware binary (esp-hal, needs the Xtensa toolchain),
`boards/stm32*`, shell/config/display services, footprint CI gate.

## Develop

```sh
cd rust
cargo test --features lama-board-heltec-v4/lora   # host tests
cargo clippy --all-targets -- -D warnings    # lint
cargo fmt --all -- --check                   # format
rustup target add thumbv7em-none-eabihf      # Cortex-M4F (STM32F3/F4)
cargo build -p lama-core --release --target thumbv7em-none-eabihf
```

## Wire format v2

7-byte little-endian header (`magic 0x42`, `version 2`, `type`, `seq u16`,
`src`, `payload_len`) followed by up to 32 payload bytes. It is **not**
byte-compatible with the C++ firmware, which sends raw structs including
compiler padding. SNR/RSSI are fixed-point centi-units (no floats on the wire).
