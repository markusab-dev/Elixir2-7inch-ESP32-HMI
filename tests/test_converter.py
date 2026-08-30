#!/usr/bin/env python3
from __future__ import annotations

import gzip
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONVERTER = ROOT / "tools" / "convert_signalk_replay.py"
UUID = "e032a164-cfe5-4cff-a347-2d671d7cc63f"


def delta(timestamp: str, values: list[dict[str, object]]) -> dict[str, object]:
    return {
        "context": f"vessels.urn:mrn:signalk:uuid:{UUID}",
        "updates": [{"timestamp": timestamp, "values": values}],
    }


def main() -> int:
    with tempfile.TemporaryDirectory() as raw_temp:
        temp = Path(raw_temp)
        source = temp / "sample.log.gz"
        output = temp / "replay_data.h"
        rows = [
            delta("2026-06-03T16:20:00.000Z", [
                {"path": "navigation.position", "value": {"latitude": 59.3, "longitude": 18.2}},
                {"path": "navigation.speedOverGround", "value": 3.0},
                {"path": "navigation.speedThroughWater", "value": 2.8},
                {"path": "navigation.headingMagnetic", "value": 2.4},
                {"path": "environment.wind.speedApparent", "value": 7.5},
                {"path": "environment.wind.angleApparent", "value": 0.7},
                {"path": "environment.depth.belowTransducer", "value": 17.2},
                {"path": "environment.water.temperature", "value": 290.15},
                {"path": "navigation.attitude", "value": {"roll": -0.1, "pitch": 0.02}},
            ]),
            delta("2026-06-03T16:20:01.000Z", [
                {"path": "navigation.position", "value": {"latitude": 59.3001, "longitude": 18.2002}},
                {"path": "navigation.speedOverGround", "value": 3.2},
                {"path": "navigation.courseOverGroundTrue", "value": 2.45},
            ]),
            delta("2026-06-03T16:20:02.000Z", [
                {"path": "navigation.position", "value": {"latitude": 59.3002, "longitude": 18.2004}},
                {"path": "navigation.speedOverGround", "value": 3.4},
            ]),
        ]
        with gzip.open(source, "wt", encoding="utf-8") as handle:
            for row in rows:
                handle.write(json.dumps(row) + "\n")

        completed = subprocess.run(
            [
                sys.executable,
                str(CONVERTER),
                str(source),
                "--output", str(output),
                "--label", "Converter smoke",
                "--sample-ms", "1000",
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0:
            print(completed.stdout)
            print(completed.stderr, file=sys.stderr)
            return completed.returncode

        text = output.read_text(encoding="utf-8")
        assert 'kReplayLabel[] = "Converter smoke"' in text
        assert "kReplayStartUnixMs" in text
        records = re.findall(r"^    \{[-0-9, ]+\},$", text, re.MULTILINE)
        assert len(records) >= 3, records
        first_values = [int(value.strip()) for value in records[0].strip(" {},").split(",")]
        assert len(first_values) == 20
        assert first_values[0] == 0
        assert first_values[1] == 593000000
        assert first_values[2] == 182000000
        assert first_values[3] > 500  # 3.0 m/s -> ~5.83 kn
        assert first_values[10] == 170  # 290.15 K -> 17.0 C

    print("test_converter: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
