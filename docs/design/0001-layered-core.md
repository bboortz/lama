# 0001 – Layered core with a minimal event kernel

Status: **Draft** · Date: 2026-11-07

## Goal

One portable core for radio-capable embedded devices (ESP32, RP2040, nRF52,
STM32/STM32WL), smallest possible footprint, testable on a PC without
hardware. LAMA becomes the first *app* on top of it.

## Non-goals (for now)

- Preemptive threads, memory protection, processes, dynamic loading.
- WiFi/BLE stacks inside the core (board/service concern).
- A second protocol: the existing LAMA packet format is the starting point.

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
│           (no hardware, no I/O)  │
├──────────────────────────────────┤
│ kernel    event queue · task     │
│           table · timer wheel ·  │
│           idle (WFI) · packet    │
│           buffer pool            │
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

1. Each layer calls only the layer below it.
2. ISRs live in `boards` and only call `kernel.post(event)`.
3. `core` depends on neither `kernel` nor hardware.
4. `services`/`apps` never include chip-specific headers.

## Kernel (scheduler only)

A run-to-completion event loop, ~200 lines:

- **Event**: `{ id, task, arg }`; large data travels as a buffer-pool index.
- **Queue**: fixed-size ring buffer; ISRs push, the main loop pops.
- **Task table**: static array of `{ id, handler, state }`.
- **Timers**: one hardware tick drives a software timer list; expiry posts an event.
- **Idle**: empty queue → `WFI`/light sleep.
- **Memory**: static pools only, no heap allocation at runtime.

Handlers never block. Waiting means "arm a timer" or "wait for an event".

Kernel API seen by tasks (the only thing a test kernel must replace):

```cpp
bool     kernel_post(Event e);                 // ISR-safe
TimerId  kernel_timer_start(TaskId t, uint32_t ms, EventId id, bool repeat);
void     kernel_timer_stop(TimerId id);
uint32_t kernel_now_ms();
```

## HAL traits (sketch)

```cpp
struct Radio {
  virtual bool begin(const RadioParams&) = 0;
  virtual bool start_rx() = 0;
  virtual bool transmit(const uint8_t* buf, size_t len) = 0;  // non-blocking
  virtual bool read(uint8_t* buf, size_t& len, RxMeta& meta) = 0; // rssi, snr, freq err
  virtual bool set_params(const RadioParams&) = 0;
};
// Storage (config), Clock, Serial, Display, Gps, Imu follow the same pattern.
```

`RadioParams` covers frequency, SF, BW, CR, sync word, preamble, TX power.
SX127x (RadioLib) and SX126x/STM32WL implement the same trait.

## Test strategy

The test kernel and the `host` board replace the real kernel and hardware.
Tasks, services and core are the **same code** in both builds.

```
 production                      test (host)
┌───────────────┐              ┌───────────────────┐
│ apps/services │              │ apps/services     │  same code
│ core          │              │ core              │  same code
├───────────────┤              ├───────────────────┤
│ real kernel   │              │ test kernel       │
│ ISR + WFI     │              │ virtual clock,    │
│ HW timer      │              │ manual stepping   │
├───────────────┤              ├───────────────────┤
│ HAL: SX127x   │              │ HAL: sim radio    │
│ board: ESP32  │              │ board: host       │
└───────────────┘              └───────────────────┘
```

- Virtual clock: `advance(10min)` runs instantly (timeouts, TX intervals, beacon loss).
- `step()` dispatches one event for assertions after each step.
- Simulated channel: N virtual nodes, drop/delay/corrupt, RSSI/SNR by distance.
- Record/replay of real field logs as regression tests.
- Levels: unit (core) → component (service + test kernel) → network → hardware-in-the-loop.

Not coverable on host: real interrupt timing, SPI/radio quirks, power behavior.

## Mapping from the current LAMA code

| Today | Target |
|---|---|
| `state.h` `systemState` (INIT/READY/IDLE/DO_RX/DO_TX/…) | `core` state machine, driven by events |
| `lora_packet.h` header/payload structs | `core/packet`, explicit (de)serialization |
| `nodes.*`, `lora_stats.*` (RSSI/SNR/loss per node) | `services/stats` (+ pure logic in `core`) |
| `lora.*` (RadioLib, SX1276 globals) | `boards/esp32` implementing `Radio` |
| `serial.*`, `cmd.ino` | `services/shell` over `Serial` trait |
| `config.*` (NVS) | `services/config` over `Storage` trait |
| `display.*` | `Display` trait + status task |
| `wifi.*` | board/service (optional) |

Issues in the current code that the refactor should address:

- Header structs are sent as raw `struct` memory; the in-file comment already
  notes the ESP32 padding (8 bytes instead of 7). The wire format must become
  explicit little-endian byte serialization with a version byte, so all chips agree.
- Globals are defined in headers (`systemState`, `radio`, `knownNodes`);
  these need to become instance state owned by a task/service.
- `String` use in state code should go (heap on small targets).
- `BEAT` payload carries a `float` (`rsnr`); define fixed-point on the wire.

## Proposed layout

```
core/        packet, state machine, link stats      (no hardware)
kernel/      event queue, tasks, timers, pools
hal/         trait headers
services/    router, stats, config, shell, log, power
apps/        lama
boards/      esp32, host  (later: rp2040, nrf52, stm32wl)
test/        host tests, network simulator
```

## Migration plan

1. Extract `core` (packet + state machine + stats) with host unit tests; no behavior change.
2. Add `kernel` + `host` board + test kernel; run the state machine on the PC.
3. Define `Radio` trait; wrap current RadioLib code as `boards/esp32`.
4. Move shell/config/display into services; LAMA becomes an app.
5. Network simulator with fault injection.
6. Second target (RP2040 or STM32WL) to prove portability.

## Open questions

- C++ core (PlatformIO + RadioLib, lowest friction from today's code) or a
  `no_std` Rust rewrite (Embassy/RTIC)? This doc assumes the architecture is
  language-independent; the C++ path keeps step 1 cheap.
- Wire format: keep `magic 0x42`/version 1 or bump to v2 with the explicit layout?
- Which ESP32 boards (pin maps) to support first?
