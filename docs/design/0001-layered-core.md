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

## HAL traits (implemented in `rust/hal`)

The traits are **synchronous and non-blocking**, matching the event-driven
state machine: calls start an operation, and completion is reported through
`poll_event()` (set by an interrupt on real hardware). This replaced the
earlier async sketch; an Embassy board can still wrap the calls in tasks.

```rust
pub trait Radio {
    type Error: core::fmt::Debug;
    fn begin(&mut self, params: &RadioParams) -> Result<(), Self::Error>;
    fn start_rx(&mut self) -> Result<(), Self::Error>;
    fn transmit(&mut self, frame: &[u8]) -> Result<(), Self::Error>;
    fn poll_event(&mut self) -> Option<RadioEvent>;          // Ready | RxReady | TxDone | Error
    fn read(&mut self, buf: &mut [u8]) -> Result<Option<(usize, RxMeta)>, Self::Error>;
}
pub trait Clock { fn now_ms(&self) -> u32; }
// Storage, Serial, Display, Gps, Imu follow the same pattern (not yet implemented).
```

`begin()` and `transmit()` only *start* the operation; `Ready` / `TxDone`
follow later, `Error` reports a hardware failure. The node arms a watchdog
(`Config::op_timeout_ms`) for every init and transmission, so a hung radio or
a lost completion interrupt becomes a Fault, then retry, then Fatal after
three consecutive faults. One `RxReady` makes the node drain all pending frames.

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
| Heltec WiFi LoRa 32 V4 (listing: Meshtastic, OLED, GPS, solar/battery input) | ESP32-S3 (Xtensa; to verify) | SX1262 (SX126x) on SPI, likely with an external front-end/PA to check in Heltec's pin docs | Second radio chip: proves the `Radio` trait across SX127x and SX126x; also covers the optional OLED/GPS/battery parts |
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

### Heltec V4 board notes (research, to verify against the schematic of the exact revision)

Sources: Heltec product page and LNA wiki, Meshtastic `heltec_v4/variant.h`,
RadioLib discussion #1665 and a TTN forum thread on V4 custom firmware.

- **SX1262 pins:** SCK 9, MISO 11, MOSI 10, NSS 8, RST 12, BUSY 13, DIO1 14.
  DIO2 drives the RF switch (`SetDio2AsRfSwitch`). TCXO on DIO3: Meshtastic
  uses 1.8 V; third-party custom-firmware reports on the V4 are inconclusive.
- **Front-end (external PA/LNA), revision dependent:**
  - Always: chip enable GPIO2 high; front-end power GPIO7 (on by default).
  - TX: PA path (CTX high, DIO2 high). RX with LNA: CTX low, DIO2 low.
    RX bypass: CTX high, DIO2 low.
  - CTX/mode pin is GPIO5 on boards with the KCT8103L (V4.3 per the Heltec
    wiki) and GPIO46 on GC1109 boards (per Meshtastic and the forum thread).
- **Other:** Vext control GPIO36 on V4 (R2), GPIO40 on V4-R8; button GPIO0;
  battery ADC GPIO1 with control GPIO37; GPS enable GPIO34, reset 42,
  PPS 41, UART TX/RX 38/39. OLED I2C pins not yet confirmed.
- **Cautions:** V4-to-V4 with RadioLib is reported working; V4 to a Waveshare
  SX1262 had unresolved receive problems in that thread. The front-end
  amplifies up to about 28 dBm, far above the EU limits: start at a low
  TX power setting and keep the antenna attached.
- **Revision handling:** the front-end type is detected at run time by reading
  GPIO2 as a floating input about 1 ms after power-up (KCT8103L pulls it high,
  GC1109 low; method from MeshCore, to be confirmed on real boards), so one
  firmware serves both. Implemented and unit-tested in
  `rust/boards/heltec-v4` (`fem.rs`). Vext (OLED) still depends on R2 vs R8.
- **Open on hardware:** how DIO2 and the mode pin interact per revision;
  verify with a TX/RX test between two boards.

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
   **Done** in `rust/core` (33 tests; builds `no_std` for `thumbv7em-none-eabihf`).
2. `hal` traits + `boards/host` (sim radio, virtual clock) + first
   multi-node simulation.
   **Done**: `rust/hal` (traits), `rust/app` (node logic), `rust/sim`
   (deterministic simulator: airtime, half-duplex, collisions, per-link loss
   and RSSI/SNR, fault injection incl. hung init and lost TX-done; 13 network
   tests). `hal` and `app` build `no_std` for `thumbv7em-none-eabihf`.
2b. **Done**: `rust/bridge` (`lama-radio-bridge`): adapts an async driver
   (`RadioDriver`, implemented over `lora-phy`) to the synchronous `Radio` trait
   with fixed-size queues, no heap (5 host tests incl. two real nodes over bridges).
2c. **Done, untested on hardware**: `boards/heltec-v4` feature `lora`:
   SX1262 driver on `lora-phy` (TCXO 1.8 V, DIO2 RF switch, DC-DC, front-end pin
   control, TX power cap). Compiles for `thumbv7em-none-eabihf` and
   `riscv32imc-unknown-none-elf`; pin logic and parameter mapping unit-tested
   with mock pins. **Not yet built for the ESP32-S3 (Xtensa).**
2d. **Next (needs the Xtensa toolchain)**: Heltec firmware binary: esp-hal
   SPI (GPIO 9/11/10, NSS 8), reset/busy/DIO1 pins, Vext/FEM power, runtime
   front-end detection (GPIO2), an Embassy executor running `bridge::run` next
   to the `Node` loop, and a defmt/serial log. Then the user flashes and reports logs.
3. `boards/esp32` with `lora-phy` + SX127x.
4. Shell/config/display services; LAMA app reaches parity with the C++ version.
5. Footprint CI gate and size budgets per board.
6. STM32F3DISCOVERY as second target (radio option A or B) to prove
   portability and the footprint budget; later RP2040, nRF52 or STM32WL.

## Open questions

- STM32F3DISCOVERY radio: option A (SX127x module) or B (UART-bridged via
  ESP32)? Exact ESP32 board model and pin map still to be listed.
- Embassy vs RTIC as the executor (Embassy: async, broad HAL support; RTIC:
  smaller, interrupt-priority model). Default: Embassy.
- Wire format: decided in step 1 as **v2**: explicit 7-byte little-endian
  header, fixed-point SNR/RSSI, not byte-compatible with the C++ firmware
  (see `rust/core/src/packet.rs`). Revisit if mixed C++/Rust networks matter.
- ESP32 variants: Xtensa (needs `espup`) or RISC-V (C3/C6)?
- OTA: image format, signing, rollback, and flash layout per board.
- Isolation: is MPU-based separation of services ever needed (untrusted or
  third-party code), or is a single trusted image enough?
