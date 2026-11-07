# 0001 – Layered Rust core with a minimal cooperative kernel

Status: **Draft (v2, Rust)** · Date: 2026-11-07

## Goal

One portable core for radio-capable embedded devices (ESP32, RP2040, nRF52,
STM32/STM32WL), with a **small footprint on every board in use**, testable on
a PC without hardware. LAMA becomes the first *app* on top of it.

## Decisions

- **Language: Rust (`no_std`, no heap).** C only where it is needed (see below).
- **Kernel: cooperative, run-to-completion, statically linked** (Embassy
  executor, or RTIC). We do not write a kernel from scratch in Rust.
- **Existing C++ LAMA code stays in the repo root** as the working reference
  until the Rust version reaches parity. The Rust workspace lives in `rust/`.

## Non-goals (for now)

- Preemptive threads, memory protection, processes, dynamic loading.
- WiFi/BLE stacks in the core (board concern, usually vendor C).
- A new protocol: the LAMA packet format is the starting point (wire format
  becomes explicit, see "Known issues").

## Layers

```
┌──────────────────────────────────┐
│ apps      lama · sensor · gateway│
├──────────────────────────────────┤
│ services  router · stats · config│
│           shell · log · power    │
│           (each = one task)      │
├──────────────────────────────────┤
│ core      packet · state machine │
│           (no_std, no hardware)  │
├──────────────────────────────────┤
│ kernel    executor · channels ·  │
│           timers · idle (WFI) ·  │
│           static buffer pool     │
├──────────────────────────────────┤
│ HAL traits  Radio · Storage ·    │
│             Clock · Serial ·     │
│             Display · GPS · IMU  │
├──────────────────────────────────┤
│ boards    esp32 · rp2040 ·       │
│           nrf52 · stm32wl · host │
│           (pins · SPI · ISRs)    │
└──────────────────────────────────┘
```

Rules:

1. Each layer depends only on the layer below it.
2. ISRs live in `boards` and only wake a task / post an event.
3. `core` depends on neither `kernel` nor hardware, so it runs in `cargo test`.
4. `services`/`apps` never import chip crates.

## Kernel type

Not a monolithic, micro, exo or hybrid kernel. Two terms describe it:

- **Library OS** (the concept): OS functions (scheduler, timers, channels,
  drivers) are libraries the app links in, not a separate kernel.
- **Unikernel** (the packaging): app + library OS are built into one
  single-purpose, single-address-space image, with no privilege separation and
  no syscalls. Each board's firmware image is one unikernel, built from shared
  crates. There is no common runtime between images; what they share is the
  platform below (the bare chip), not a runtime.

By size and feature set the kernel part is **nanokernel**-class (scheduling,
timers, events, nothing else).

- Services are tasks with typed channels between them: microkernel-*shaped*
  structure, but without isolation or user-space servers.
- Hardware is reached directly through HAL traits: exokernel-like in spirit
  (library-OS lineage), but there is no protection layer.
- Isolation is a possible later step on chips with an MPU (see Tock and
  Hubris for Rust examples); the channel-based design carries over.

Scheduling model: cooperative async tasks (Embassy) or interrupt-priority
tasks (RTIC). Handlers never block. The CPU sleeps (`WFI`) when idle.
No heap; packet buffers come from static pools and travel by index/reference.

## HAL traits (sketch)

```rust
pub trait Radio {
    async fn begin(&mut self, p: &RadioParams) -> Result<(), RadioError>;
    async fn transmit(&mut self, buf: &[u8]) -> Result<(), RadioError>;
    async fn receive(&mut self, buf: &mut [u8]) -> Result<RxMeta, RadioError>; // rssi, snr, freq err
    async fn set_params(&mut self, p: &RadioParams) -> Result<(), RadioError>;
}
// Storage, Clock, Serial, Display, Gps, Imu follow the same pattern.
```

`RadioParams`: frequency, SF, BW, CR, sync word, preamble, TX power.
SX127x (ESP32 boards today) and SX126x/STM32WL implement the same trait,
backed by the `lora-phy` crate.

## Where C is still used

C is allowed only where Rust has no practical option:

- **Vendor radio/connectivity blobs:** ESP-IDF WiFi/BLE (if needed on ESP32),
  Nordic SoftDevice (if BLE is needed on nRF52).
- **RadioLib via FFI**, only as a fallback if `lora-phy` misbehaves on a
  specific SX127x board.
- **Boot/startup code** supplied by the chip vendor/HAL crates.

All C sits behind the same HAL traits, in a board crate, never in `core`,
`services` or `apps`.

## Target boards (first two)

| Board | Chip | Radio | Role |
|---|---|---|---|
| ESP32 + SX127x (existing LAMA hardware) | ESP32 (variant to confirm) | SX127x on SPI | First real radio target |
| STM32F3DISCOVERY | STM32F303VC, Cortex-M4F 72 MHz, 256 KB flash, ~48 KB RAM, on-board ST-LINK/V2 | none on board | Second target: proves portability and the small footprint |
| STM32F4 "Black Pill" clone (FEIYANG V3.0, USB-C) | F401CCU6 (84 MHz, 256 KB flash, 64 KB RAM) or F411CEU6 (100 MHz, 512 KB flash, 128 KB RAM); variant to confirm | none on board | Third target / cheap radio host; no on-board debugger, flash via SWD header (e.g. the F3DISCOVERY's ST-LINK) or USB DFU |

STM32F3DISCOVERY radio options:

- **A (preferred):** external SX127x module (RFM95/Ra-02 class) on a free SPI
  bus (the on-board gyro uses SPI1, to be checked in the board manual), so
  both boards share the same `lora-phy` SX127x driver.
- **B (fallback, no purchase):** UART-bridged `Radio`: the STM32 forwards
  frames to the ESP32, which does the RF.

The on-board gyro and accelerometer/magnetometer double as the first `Imu`
trait implementation.

Pass criteria for "common ground works": same `core`/`services` crates build
for both chips with no `cfg` outside board crates; the boards exchange
BEAT/PING/PONG with correct RSSI/SNR/loss stats; flash/RAM within budget.

## Footprint policy

The smallest board in use sets the budget. With the STM32F3DISCOVERY that is
**256 KB flash and ~48 KB RAM** (verify against the datasheet; part of the RAM
is CCM, which DMA cannot reach). ESP32 and other candidate chips (RP2040,
nRF52, STM32WL) have the same or more room.

Rules to keep it small:

- `no_std`, no `alloc` in `core`, `kernel`, `services`, `apps`.
- Static pools and fixed-size queues; capacities are compile-time constants.
- Release profile: `opt-level = "z"`, `lto = "fat"`, `codegen-units = 1`,
  `panic = "abort"`.
- Logging via `defmt` (no `core::fmt` on target) and behind a feature flag.
- Cargo features per service, so unused services cost nothing.
- Fixed-point on the wire and in stats; no floating point on chips without an FPU.
- **CI size gate:** build every board in release mode and fail on flash/RAM
  budget regressions (`cargo size` / `cargo bloat`); budgets are per board and
  stored in the repo.

## Images (one unikernel per device)

Each device role is a separate image built from the same crates; Cargo
features select which services are linked in.

| Image | Services linked in (example) |
|---|---|
| `lama` (analyzer) | router, stats, shell, config, display |
| `sensor-node` | router, config, power, sensor task |
| `gateway` | router, stats, log, serial/WiFi bridge |

Consequences:

- Footprint budgets are per image and per board, enforced in CI.
- Updates ship as a full, signed image (OTA design is an open question).
- Debugging is by probe and logs (`probe-rs` + `defmt`), not an on-target
  shell; the serial shell is a service that can be left out.

## Test strategy

The `host` board replaces hardware; the same tasks, services and core run on
the PC.

```
 production                      test (host)
┌───────────────┐              ┌───────────────────┐
│ apps/services │              │ apps/services     │  same code
│ core          │              │ core              │  same code
├───────────────┤              ├───────────────────┤
│ Embassy/RTIC  │              │ host executor     │
│ ISR + WFI     │              │ virtual clock,    │
│ HW timer      │              │ manual stepping   │
├───────────────┤              ├───────────────────┤
│ HAL: lora-phy │              │ HAL: sim radio    │
│ board: ESP32  │              │ board: host       │
└───────────────┘              └───────────────────┘
```

- Virtual clock: advance minutes of time instantly (timeouts, TX intervals,
  beacon loss). To be verified: Embassy's `embassy-time` mock driver; if it
  does not fit, the kernel keeps a thin time trait we can fake.
- Simulated channel: N virtual nodes, drop/delay/corrupt, RSSI/SNR by distance.
- Record/replay of real field logs as regression tests.
- Property tests (`proptest`) for packet encode/decode.
- Levels: unit (core) → component → network → hardware-in-the-loop.

Not coverable on host: real interrupt timing, SPI/radio quirks, power behavior.

## Mapping from the current C++ LAMA code

| Today (C++) | Target (Rust) |
|---|---|
| `state.h` `systemState` (INIT/READY/IDLE/DO_RX/DO_TX/…) | `core` state machine |
| `lora_packet.h` header/payload structs | `core::packet`, explicit (de)serialization |
| `nodes.*`, `lora_stats.*` | `services::stats` (+ pure logic in `core`) |
| `lora.*` (RadioLib, SX1276 globals) | `boards/esp32` implementing `Radio` via `lora-phy` |
| `serial.*`, `cmd.ino` | `services::shell` over `Serial` trait |
| `config.*` (NVS) | `services::config` over `Storage` trait |
| `display.*` | `Display` trait + status task |
| `wifi.*` | board/service, optional, C where required |

## Known issues in the current code (to fix, not port)

- Header structs are sent as raw memory; the code comment already notes ESP32
  padding (8 bytes instead of 7). The wire format becomes explicit
  little-endian byte layout with a version byte.
- Globals defined in headers (`systemState`, `radio`, `knownNodes`) become
  state owned by a task/service.
- `String` use in state code (heap) disappears.
- BEAT payload carries a `float` (`rsnr`); use fixed-point on the wire.

## Proposed layout

```
rust/
  Cargo.toml            workspace
  core/                 packet, state machine, link stats      (no hardware)
  hal/                  trait definitions
  kernel/               channels, timers, pools (thin layer over Embassy/RTIC)
  services/             router, stats, config, shell, log, power
  apps/lama/            first app
  boards/{host,esp32}/  later: rp2040, nrf52, stm32wl
  sim/                  network simulator + fault injection
*.ino, *.h              existing C++ reference (unchanged until parity)
```

## Migration plan

1. Workspace + `core` (packet, state machine, stats) with host unit tests.
2. `hal` traits + `boards/host` (sim radio, virtual clock) + first
   multi-node simulation.
3. `boards/esp32` with `lora-phy` + SX127x; user flashes and reports logs.
4. Shell/config/display services; LAMA app reaches parity with the C++ version.
5. Footprint CI gate and size budgets per board.
6. STM32F3DISCOVERY as second target (radio option A or B) to prove
   portability and the footprint budget; later RP2040, nRF52 or STM32WL.

## Open questions

- STM32F3DISCOVERY radio: option A (SX127x module) or B (UART-bridged via
  ESP32)? Exact ESP32 board model and pin map still to be listed.
- Embassy vs RTIC as the executor (Embassy: async, broad HAL support; RTIC:
  smaller, interrupt-priority model). Default: Embassy.
- Wire format: bump to v2 with the explicit layout, or stay on v1?
- ESP32 variants: Xtensa (needs `espup`) or RISC-V (C3/C6)?
- OTA: image format, signing, rollback, and flash layout per board.
- Isolation: is MPU-based separation of services ever needed (untrusted or
  third-party code), or is a single trusted image enough?
