# LAMA Rust workspace

Work in progress, see [`docs/design/0001-layered-core.md`](../docs/design/0001-layered-core.md).
The C++ firmware in the repository root stays the working reference until the
Rust version reaches parity.

## Crates

| Crate | What | Status |
|---|---|---|
| `core` (`lama-core`) | `no_std`, dependency-free: wire format v2 (`packet`), link statistics (`stats`), TX/RX state machine (`state`) | implemented, host-tested |

Planned next: `hal` (traits), `kernel`, `boards/host` (simulated radio and
virtual clock), `boards/esp32`, `boards/stm32*`, `sim`.

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
