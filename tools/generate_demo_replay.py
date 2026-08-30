#!/usr/bin/env python3
"""Generate the built-in 12-minute demo sail."""
from __future__ import annotations
import math
from pathlib import Path
from convert_signalk_replay import Sample, write_header


def main() -> None:
    start_ms = 1786220400000
    samples: list[Sample] = []
    for second in range(0, 12 * 60 + 1, 2):
        t = float(second)
        slow = math.sin(t * 0.026)
        medium = math.sin(t * 0.093 + 0.8)
        quick = math.sin(t * 0.31 + 1.2)
        heading = (138.0 + 9.5 * slow + 2.0 * quick) % 360.0
        sog = 6.35 + 0.68 * slow + 0.20 * quick
        samples.append(Sample(
            timestamp_ms=start_ms + second * 1000,
            latitude=59.2944 + second * 0.000012 + 0.00025 * math.sin(t * 0.01),
            longitude=18.2230 + second * 0.000020 + 0.00018 * math.cos(t * 0.013),
            sog_kn=sog,
            stw_kn=sog - 0.22 + 0.12 * medium,
            cog_deg=(heading - 2.2 + 1.5 * medium) % 360.0,
            heading_deg=heading,
            aws_kn=14.7 + 2.9 * medium + 0.75 * quick,
            awa_deg=39.0 + 14.0 * slow - 3.0 * quick,
            depth_m=22.0 + 10.0 * math.sin(t * 0.014 + 0.6),
            water_temp_c=17.4 + 0.12 * math.sin(t * 0.004),
            roll_deg=-7.4 + 2.5 * medium + 1.0 * quick,
            pitch_deg=0.7 + 1.0 * math.sin(t * 0.20), recognized_values=12,
        ))
    root = Path(__file__).resolve().parents[1]
    output = root / "src" / "generated" / "replay_data.h"
    write_header(output, samples, "Kvallssegling demo", 120)
    print(f"Wrote {len(samples)} demo records to {output}")


if __name__ == "__main__":
    main()
