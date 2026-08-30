# Architecture

## Core rule

Every producer writes the same `BoatState`; every consumer reads the same `BoatState`.

```text
SyntheticDataSource ─┐
ReplayDataSource ────┼──> DataEngine ──> BoatState ──┬──> LVGL UI
Future NMEA2000 ─────┤                                ├──> HistoryBuffer
Future Victron Wi-Fi ┤                                ├──> diagnostics
Future ESP-NOW temps ┘                                └──> logger/uploader
```

The v0.1 data source can therefore be swapped without page-specific simulation code.

## Scheduling

The Arduino loop is deliberately simple and deterministic:

- data interpolation: 25 Hz,
- numeric UI refresh: 10 Hz,
- history sampling: 1 Hz,
- system telemetry: 1 Hz,
- LVGL rendering: its own FreeRTOS task.

All external LVGL calls are protected by a recursive mutex. The LVGL rendering task owns `lv_timer_handler()`.

## Replay format

`ReplayRecordPacked` is 32 bytes and stores fixed-point values. The packed format avoids JSON parsing and dynamic allocation on the microcontroller.

Current v0.1 replay is compiled into flash. A later SD replay reader can use the same layout and decoder with a block cache instead of changing the UI or interpolation layer.

## Future live inputs

### NMEA2000

Add a `LiveNmeaDataSource` or a background producer that updates a synchronized live-state cache. CAN reception must have its own high-priority ring buffer and never block on UI, SD or Wi-Fi.

### Victron

The Pi Zero 2W remains connected by USB to SmartShunt and SmartSolar. The ESP32 should consume its existing Wi-Fi stream and map it to the battery/solar fields in `BoatState`.

### Temperatures

Register the Waveshare station as an ESP-NOW peer/receiver and map node IDs to engine, alternator, fridge and cabin channels. ESPNOW callbacks should only enqueue compact packets; parsing/state updates should happen outside the radio callback.

## Logging direction

The intended production pipeline is:

```text
CAN frames / Victron / ESP-NOW
          ↓
lossless queues
          ↓
compact segmented raw log
          ↓
CRC + compression
          ↓
microSD
          ↓
outbound upload queue over boat Wi-Fi
```

Firmware remains in internal flash. microSD is data-only, so a damaged data card cannot prevent the ESP32 from booting into diagnostics.
