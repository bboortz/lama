# lama
LAMA (LoRa Analytics and Monitoring Apparatus)

Remote-controlled LoRa mesh network analyzer and quality monitoring system

Overview
LAMA is a WiFi-enabled LoRa transceiver designed for comprehensive network quality analytics in LoRa mesh deployments. The device combines long-range LoRa communication with WiFi connectivity, allowing remote configuration, monitoring, and data collection without physical access to the device.
LAMA helps you systematically discover and map LoRa network coverage and performance characteristics.


# Features


* Network Quality Analytics
  * RSSI (Received Signal Strength Indicator) monitoring
  * SNR (Signal-to-Noise Ratio) measurement
  * Packet loss rate calculation


* Remote Control via WiFi
  * Web-based configuration interface
  * Real-time statistics dashboard
  * RESTful API for automation


* LoRa Parameter Tuning
  * Adjustable spreading factor (SF7-SF12)
  * Configurable bandwidth
  * Transmission power control
  * Frequency band selection



# Hardware Requirements

* Microcontroller: ESP32 (WiFi + Bluetooth)
* LoRa Module: SX1276/SX1278 based transceiver
* Antenna: 868 MHz (EU) or 915 MHz (US/AU) depending on region
* Power Supply or battery
* optional
  * OLED display
  * GPS module
  * movement sensor


# Internal Architecture

## Overview

LAMA follows a **non-blocking state machine** architecture with modular components for network communication, configuration management, and user interfaces. The system is designed to handle concurrent LoRa operations, WiFi connectivity, and user interaction without blocking.

## Core Components

### State Management
* **System State Machine** - Controls operational flow (INIT → READY → IDLE → DO_TX/DO_RX)
* **Network State Tracker** - Monitors LoRa connectivity (CONNECTED/LOST)
* **Non-volatile Configuration** - Persistent settings stored in ESP32 NVS (Preferences)

### Network Interfaces
* **LoRa Transceiver (RW)**
  * Interrupt-driven packet reception via RadioLib (SX1276/SX1278)
  * Configurable RF parameters (frequency, SF, BW, TX power)
  * Packet encoding/decoding with loss detection
* **WiFi Interface (RW)**
  * Station mode for network connectivity
  * Access Point mode for initial configuration
  * Web server for remote control (planned)
  * RESTful API endpoints (planned)

### Visual Interfaces
* **Serial Console (RW)**
  * Command-line interface for configuration
  * Real-time statistics and diagnostics
  * Debug logging
* **OLED Display (RO)**
  * Network status visualization
  * Signal quality metrics (RSSI, SNR)
  * Packet statistics and loss rates

### Data Management
* **Packet System**
  * Structured packet types (BEAT, PING, PONG, DISCOVER, etc.)
  * User-based packet tracking
  * Signal quality metrics per sender
* **RX History**
  * Circular buffer of received packets
  * Per-user statistics (packets received, lost, RSSI avg/min/max)
  * Signal degradation warnings

### Command Processing
* **Serial Command Handler**
  * Modular command dispatch system
  * Separate handler functions per command category
  * Runtime configuration updates
  * System control (reboot, reset, stats)

## Architecture Diagram
```
┌─────────────────────────────────────────────────────────────────────┐
│                        LAMA System Architecture                      │
└─────────────────────────────────────────────────────────────────────┘

Input Sources                  Core System                  Output Interfaces
─────────────                 ────────────                 ─────────────────

┌──────────┐                                               ┌──────────┐
│  Serial  │──┐                                         ┌─→│  Serial  │
│ Console  │  │                                         │  │  Output  │
└──────────┘  │                                         │  └──────────┘
              │    ┌─────────────────────────┐          │
┌──────────┐  │    │   Command Dispatcher    │          │  ┌──────────┐
│   WiFi   │──┼───→│   (Serial Commands)     │──────────┼─→│  OLED    │
│  Client  │       └─────────────────────────┘          │  │ Display  │
└──────────┘                 │                          │  └──────────┘
                             ↓                          │
┌──────────┐       ┌─────────────────────────┐          │  ┌──────────┐
│   LoRa   │       │    State Machine        │          └─→│  LoRa    │
│   RX     │       │  ┌────────────────────┐ │             │   TX     │
│(Interrupt│←──────┼──│ INIT → READY       │ │────────────→│(Transmit)│
│ Driven)  │       │  │ IDLE ⇄ DO_RX       │ │             └──────────┘
└──────────┘       │  │ IDLE ⇄ DO_TX       │ │
                   │  └────────────────────┘ │             ┌──────────┐
                   └─────────────────────────┘             │   WiFi   │
                              │                            │  Server  │
                              ↓                            │ (planned)│
                   ┌─────────────────────────┐             └──────────┘
                   │  Configuration Manager  │
                   │  ┌──────────────────┐   │
                   │  │   NVS Storage    │   │
                   │  │  (Preferences)   │   │
                   │  └──────────────────┘   │
                   └─────────────────────────┘
                              │
                   ┌──────────┴──────────┐
                   ↓                     ↓
        ┌──────────────────┐  ┌──────────────────┐
        │  Packet Manager  │  │   RX History     │
        │  ┌────────────┐  │  │  ┌────────────┐  │
        │  │ Encode     │  │  │  │ Per-User   │  │
        │  │ Decode     │  │  │  │ Statistics │  │
        │  │ Validate   │  │  │  │ Loss Track │  │
        │  └────────────┘  │  │  └────────────┘  │
        └──────────────────┘  └──────────────────┘
```

## Data Flow

### Transmission Flow
```
User Command → State Machine (DO_TX) → Encode Packet → LoRa TX → Update Stats
```

### Reception Flow (Interrupt-Driven)
```
LoRa Interrupt → Set Flag → State Machine (DO_RX) → Decode Packet → 
→ Update User Stats → RX History → Display Update → Return to IDLE
```

### Configuration Flow
```
Serial/WiFi Input → Command Dispatcher → Validate → Update Config → 
→ Save to NVS → Acknowledge → (Reboot if required)
```

## Key Design Principles

1. **Non-Blocking Operation**
   * Interrupt-driven LoRa reception
   * State machine prevents blocking delays
   * Async WiFi operations

2. **Modular Command System**
   * Each command has dedicated handler function
   * Easy to add new commands
   * Centralized validation

3. **Persistent Configuration**
   * All settings stored in NVS
   * Survives reboots and power loss
   * Runtime reconfigurable

4. **Signal Quality Tracking**
   * Per-user statistics (RSSI, SNR, packet loss)
   * Moving averages for trend detection
   * Automatic degradation warnings

5. **Extensible Packet Protocol**
   * Multiple packet types for different functions
   * Easy to add new packet types
   * Backward-compatible parsing

## State Machine Details
```
┌──────┐
│ INIT │ - System initialization, load config
└───┬──┘
    │
    ↓
┌───────┐
│ READY │ - Initialization complete
└───┬───┘
    │
    ↓
┌──────┐ ←──────────────┐
│ IDLE │                │
└───┬──┘                │
    │                   │
    ├─→ DO_RX → IN_RX ──┤ (Packet received via interrupt)
    │                   │
    └─→ DO_TX → IN_TX ──┘ (TX interval elapsed)
```

## Configuration Storage

All configuration parameters are stored persistently using ESP32's NVS (Non-Volatile Storage):

* **Node Configuration**: ID, network ID, capabilities, username
* **LoRa Parameters**: Frequency, bandwidth, SF, CR, sync word, preamble, TX power
* **Pin Assignments**: OLED pins, LoRa SPI pins
* **Timing**: TX interval, RX timeout
* **WiFi Settings**: SSID, password, hostname, enabled state

## Future Enhancements

* Web-based configuration interface
* RESTful API for remote control
* MQTT integration for cloud logging
* GPS support for coverage mapping
* Multi-device mesh topology analysis
* Machine learning for link quality prediction

