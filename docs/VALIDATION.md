# Validation record

## Completed in the generated project

- C++17 host build with `-Wall -Wextra -Wpedantic -Werror` for:
  - `DataEngine`,
  - `ReplayDataSource`,
  - `SyntheticDataSource`,
  - `HistoryBuffer`,
  - packed replay format.
- Host smoke test passed.
- Signal K `.log.gz` converter smoke test passed.
- Python bytecode compilation passed.
- `UiTheme.cpp` and `ElixirUI.cpp` passed host C++ syntax compilation against an LVGL v8-shaped test stub.
- Replay record size is compile-time asserted to 32 bytes.
- 8 MB and 16 MB partition tables are checked by `tools/verify_project.py`.

Run all local checks with:

```bash
python3 tools/verify_project.py
```

## Not completed in this environment

The following require the real PlatformIO ESP32 toolchain and/or the physical Waveshare board:

- complete Xtensa/Arduino/ESP-IDF link,
- final firmware binary size,
- flash and PSRAM auto-detection,
- RGB timing and screen-drift validation,
- touch orientation,
- physical backlight behavior,
- long-duration watchdog/thermal testing.

Those items must not be represented as passed until the first hardware run.

## First hardware results

Fill this section after flashing:

```text
Date:
Board flash size:
PSRAM total/free:
Firmware binary size:
Touch orientation:
Display drift/tearing:
Backlight off/wake:
30-minute stability:
Notes:
```

## 0.1.1 layout correction

- User-visible vessel name changed from `ELIXIR` to `ELIXIR2` throughout the UI, previews and handoff documentation.
- Apparent-wind number, unit and AWA row were raised; the true-wind inset was lowered and reduced slightly in height.
- The AWA label now has an explicit 210 px width in the LVGL implementation.
- The preview renderer contains a regression assertion requiring at least 7 px clearance between the AWA text bounds and the true-wind inset.
- All host checks passed again after the correction.
