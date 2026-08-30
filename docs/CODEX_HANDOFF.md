# Codex handoff: first hardware bring-up

## Objective

Compile, flash and run the existing v0.1.1 on the exact **Waveshare ESP32-S3-Touch-LCD-7**. Preserve the visual design and data architecture. Make only the hardware/toolchain corrections proven necessary by the actual board.

## First commands

```bash
pio run -e waveshare_16mb
pio run -e waveshare_16mb -t upload
pio device monitor -b 115200
```

If flash detection or upload indicates an 8 MB module:

```bash
pio run -e waveshare_8mb
pio run -e waveshare_8mb -t upload
```

## Expected serial output

```text
[ELIXIR2 HMI] firmware 0.1.1-layout
[ELIXIR2 HMI] GUI + synthetic/recorded data evaluation
[app] ready — touch the bottom navigation or open SYSTEM
```

## Hardware acceptance checklist

- [ ] 800×480 landscape image fills the panel.
- [ ] No screen drift, tearing, repeated bands or color-channel swap.
- [ ] Touch hits all five bottom navigation regions accurately.
- [ ] Replay controls work and the slider seeks.
- [ ] `SYSTEM` shows 8 MB-class PSRAM and plausible free heap.
- [ ] Physical backlight turns off and touch wakes it.
- [ ] No watchdog reset after at least 30 minutes at 30× replay.
- [ ] `pio run -t size` is recorded in `docs/VALIDATION.md`.

## Likely integration points

1. `src/hal/LvglPort.cpp`
   - If Waveshare's current example requires a bounce buffer or anti-tearing port, replace only this implementation while retaining `LvglPort.h`.
   - Do not move UI code into the HAL.
2. `platformio.ini`
   - Keep Arduino-ESP32 and all libraries pinned unless a concrete compiler incompatibility requires a coordinated update.
3. `include/esp_panel_board_supported_conf.h`
   - Must continue selecting `BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7`.
4. `src/hal/DisplayRuntime.cpp`
   - Confirm the two USB-C connectors and use the USB-UART/programming path appropriate for the board.

## Do not implement during bring-up

- NMEA2000,
- SD logging,
- Wi-Fi/Victron,
- ESP-NOW,
- visual redesign,
- large framework migration.

First establish a stable display/touch/replay baseline. Commit that known-good baseline before live integrations.

## Real-log replay

After the demo runs, generate a replay from a known-good Signal K interval using the command in the root README. The output intentionally replaces `src/generated/replay_data.h`.

## Next production milestones

1. Live listen-only NMEA2000 state update.
2. Robust segmented raw logger to microSD.
3. Victron Wi-Fi adapter.
4. ESP-NOW temperature receiver.
5. Local HTTP status/download endpoint.
6. Compression and outbound upload queue.
7. Long-duration corruption/power-cut tests.
