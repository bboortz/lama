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
| `sim` (`lama-sim`) | Deterministic network simulator: virtual clock, airtime, half-duplex, collisions, per-link loss and RSSI/SNR, fault injection | implemented, 10 tests |

Planned next: `boards/esp32` (SX126x/SX127x via `lora-phy`), `boards/stm32*`,
shell/config/display services, footprint CI gate.

## Develop

```sh
cd rust
cargo test                                   # host tests
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
