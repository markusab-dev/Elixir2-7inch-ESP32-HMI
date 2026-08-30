# Elixir2 HMI v0.1.1

A first, hardware-targeted firmware draft for the **Waveshare ESP32-S3-Touch-LCD-7** (800×480). It is designed to answer one question before the boat integration begins:

> Does this device feel like a polished, fast and useful permanent boat-data display?

The firmware boots straight into a five-page LVGL dashboard, supports touch navigation, runs either a synthetic sailing feed or a replay generated from real Signal K logs, keeps a rolling chart history, exposes system/memory information, and can turn the physical backlight fully off with touch-to-wake.

## What is implemented

- Exact Waveshare 7-inch board profile through `ESP32_Display_Panel`.
- 800×480 capacitive touch UI using LVGL 8.4.
- Five pages:
  - **LIVE** — SOG, STW, apparent/true wind, heading, COG, TWD, depth, water temperature, heel and VMG.
  - **POWER** — simulated Victron-compatible state model: SOC, voltage, current, power, solar and autonomy.
  - **TEMPS** — engine, alternator, fridge and cabin temperatures.
  - **HISTORY** — rolling SOG/STW/AWS history.
  - **SYSTEM** — replay controls, source switching, night overlay, visual dimming, screen off, flash/PSRAM/heap/uptime.
- Built-in 12-minute recorded-style demo replay at 0.5×, 1×, 2×, 5×, 10× and 30×.
- Converter for actual Signal K `.log` and `.log.gz` files.
- A compact 32-byte replay record format that can later be read directly from microSD.
- 16 MB and 8 MB flash partition profiles.
- Host-side tests for the data engine, history buffer and Signal K converter.

## Deliberate v0.1 limits

This release is a display/replay evaluation build. It does **not yet** connect to:

- live NMEA2000/CAN,
- the Pi Zero 2W Victron stream over Wi-Fi,
- ESP-NOW temperature nodes,
- microSD logging,
- automatic upload.

Those inputs already have stable interfaces in the code. The UI consumes a single `BoatState`, so replacing the demo producers does not require redesigning the screens.

The top-bar N2K/Wi-Fi/SD dots therefore remain grey in v0.1 by design.

## Build with PlatformIO

Open the project root in VS Code/PlatformIO and run:

```bash
pio run -e waveshare_16mb
pio run -e waveshare_16mb -t upload
pio device monitor -b 115200
```

If the board reports 8 MB flash or the 16 MB image cannot be flashed:

```bash
pio run -e waveshare_8mb
pio run -e waveshare_8mb -t upload
```

The first useful hardware checks are:

1. UI fills the entire 800×480 display and colors are correct.
2. Touch coordinates match the visible controls.
3. The replay runs smoothly and charts update without drift or tearing.
4. `SYSTEM` reports approximately 8 MB PSRAM.
5. `SCREEN OFF` cuts the physical backlight and the next touch wakes it.

## Replay a real saved sailing log

The converter understands the Signal K paths already used by Boatdata, including position, SOG, STW, COG, heading, apparent wind, attitude, depth and water temperature. It reads `.log` and `.log.gz` directly.

For the previously identified 3 June test interval, Swedish local time 18:20–21:30 corresponds approximately to 16:20–19:30 UTC:

```bash
python3 tools/convert_signalk_replay.py \
  "/Users/testadmin/Desktop/NMEA2000 loggar/logs" \
  --output src/generated/replay_data.h \
  --label "Elixir2 – 3 juni" \
  --start "2026-06-03T16:20:00Z" \
  --end "2026-06-03T19:30:00Z" \
  --sample-ms 1000 \
  --timezone-offset-min 120
```

Then rebuild and flash. The converter defaults to the known own-boat UUID/MMSI contexts, but `--context` can be supplied repeatedly when needed.

For a quick converter-only test:

```bash
python3 tests/test_converter.py
```

## Memory expectations

The replay format is exactly **32 bytes per sample**:

- built-in 12-minute demo at 2-second resolution: about 11.6 KB,
- three hours at 1 Hz: about 346 KB,
- three hours at 2 Hz: about 691 KB.

The complete firmware is expected to land roughly in the **2.5–4 MB** region because LVGL, board drivers, Wi-Fi-capable Arduino core and multiple Montserrat font sizes dominate the image. The exact figure must be taken from the first real PlatformIO build; it has not been fabricated here.

PSRAM use is dominated by display/LVGL buffers and should remain comfortably below the available 8 MB in this first version.

## Screen brightness behavior

The board profile exposes the backlight as an I/O-expander **switch**, not hardware PWM. Consequently:

- `SCREEN OFF` is a real physical backlight shutdown.
- Touch remains active for wake-up.
- `DIM 45/20/8%` is a software black-overlay preview of night readability, not reduced LED power consumption.
- `NIGHT MODE` adds a warm red filter independently of the dim level.

True analog/PWM backlight dimming would require a small hardware modification or a different backlight control path. The software API is isolated so that can be added without redesigning the UI.

## Project map

```text
src/app/          application scheduling and UI commands
src/data/         synthetic/replay data sources and derived wind values
src/generated/    compiled replay generated from Signal K logs
src/hal/          Waveshare display, touch, backlight and LVGL port
src/model/        shared state and rolling history
src/ui/           polished five-page LVGL interface
tools/            replay conversion and demo generation
tests/            host smoke tests and syntax checks
docs/             architecture, validation and Codex handoff
preview/          visual design previews matching the implemented layout
```

See `docs/CODEX_HANDOFF.md` before the first hardware build.
