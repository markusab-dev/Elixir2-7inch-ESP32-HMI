#!/usr/bin/env python3
"""Convert Signal K delta .log/.log.gz files to a compact firmware replay."""
from __future__ import annotations

import argparse
import gzip
import json
import math
import sys
from dataclasses import dataclass, replace
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Iterator, TextIO

MS_TO_KNOTS = 1.9438444924406
RAD_TO_DEG = 180.0 / math.pi


@dataclass
class Sample:
    timestamp_ms: int
    latitude: float = 0.0
    longitude: float = 0.0
    sog_kn: float = 0.0
    stw_kn: float = 0.0
    cog_deg: float = 0.0
    heading_deg: float = 0.0
    aws_kn: float = 0.0
    awa_deg: float = 0.0
    depth_m: float = 0.0
    water_temp_c: float = 0.0
    roll_deg: float = 0.0
    pitch_deg: float = 0.0
    battery_soc_pct: float = 0.0
    battery_voltage_v: float = 0.0
    battery_current_a: float = 0.0
    solar_power_w: float = 0.0
    engine_temp_c: float = 0.0
    alternator_temp_c: float = 0.0
    engine_room_temp_c: float = 0.0
    recognized_values: int = 0


def parse_datetime(value: str | None) -> datetime | None:
    if not value:
        return None
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return parsed.astimezone(timezone.utc)


def unix_ms(value: datetime) -> int:
    return int(round(value.timestamp() * 1000.0))


def normalize_degrees(value: float) -> float:
    return value % 360.0


def discover_files(inputs: Iterable[str]) -> list[Path]:
    result: list[Path] = []
    for raw in inputs:
        path = Path(raw).expanduser()
        if path.is_dir():
            result.extend(path.rglob("*.log"))
            result.extend(path.rglob("*.log.gz"))
        elif path.is_file():
            result.append(path)
        else:
            result.extend(Path().glob(raw))
    unique = sorted({item.resolve() for item in result})
    return [item for item in unique if item.name.endswith((".log", ".log.gz"))]


def open_text(path: Path) -> TextIO:
    if path.name.endswith(".gz"):
        return gzip.open(path, "rt", encoding="utf-8", errors="replace")
    return path.open("rt", encoding="utf-8", errors="replace")


def context_matches(context: str, filters: list[str]) -> bool:
    return not filters or any(token in context for token in filters)


def update_state(state: Sample, path: str, value: object) -> bool:
    try:
        if path == "navigation.position" and isinstance(value, dict):
            latitude = value.get("latitude")
            longitude = value.get("longitude")
            if latitude is not None:
                state.latitude = float(latitude)
            if longitude is not None:
                state.longitude = float(longitude)
            return latitude is not None or longitude is not None
        if path == "navigation.speedOverGround":
            state.sog_kn = float(value) * MS_TO_KNOTS
        elif path == "navigation.speedThroughWater":
            state.stw_kn = float(value) * MS_TO_KNOTS
        elif path in ("navigation.courseOverGroundTrue", "navigation.courseOverGroundMagnetic"):
            state.cog_deg = normalize_degrees(float(value) * RAD_TO_DEG)
        elif path in ("navigation.headingMagnetic", "navigation.headingTrue"):
            state.heading_deg = normalize_degrees(float(value) * RAD_TO_DEG)
        elif path == "environment.wind.speedApparent":
            state.aws_kn = float(value) * MS_TO_KNOTS
        elif path == "environment.wind.angleApparent":
            state.awa_deg = float(value) * RAD_TO_DEG
        elif path in ("environment.depth.belowTransducer", "environment.depth.belowSurface"):
            state.depth_m = float(value)
        elif path == "environment.water.temperature":
            numeric = float(value)
            state.water_temp_c = numeric - 273.15 if numeric > 100.0 else numeric
        elif path == "navigation.attitude" and isinstance(value, dict):
            if value.get("roll") is not None:
                state.roll_deg = float(value["roll"]) * RAD_TO_DEG
            if value.get("pitch") is not None:
                state.pitch_deg = float(value["pitch"]) * RAD_TO_DEG
        elif path == "electrical.batteries.house.capacity.stateOfCharge":
            numeric = float(value)
            state.battery_soc_pct = numeric * 100.0 if numeric <= 1.0 else numeric
        elif path == "electrical.batteries.house.voltage":
            state.battery_voltage_v = float(value)
        elif path == "electrical.batteries.house.current":
            state.battery_current_a = float(value)
        elif path == "electrical.chargers.solar.power":
            state.solar_power_w = float(value)
        elif path == "propulsion.mainEngine.temperature":
            numeric = float(value)
            state.engine_temp_c = numeric - 273.15 if numeric > 100.0 else numeric
        elif path == "electrical.alternators.0.temperature":
            numeric = float(value)
            state.alternator_temp_c = numeric - 273.15 if numeric > 100.0 else numeric
        elif path == "environment.inside.engineRoom.temperature":
            numeric = float(value)
            state.engine_room_temp_c = numeric - 273.15 if numeric > 100.0 else numeric
        else:
            return False
    except (TypeError, ValueError, OverflowError):
        return False
    return True


def iter_updates(
    files: list[Path], contexts: list[str], end_ms: int | None
) -> Iterator[tuple[int, list[dict[str, object]]]]:
    for path in files:
        print(f"Reading {path}", file=sys.stderr)
        with open_text(path) as handle:
            for line in handle:
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                except json.JSONDecodeError:
                    continue
                context = str(obj.get("context", ""))
                if not context_matches(context, contexts):
                    continue
                updates = obj.get("updates")
                if not isinstance(updates, list):
                    continue
                for update in updates:
                    if not isinstance(update, dict):
                        continue
                    timestamp_raw = update.get("timestamp")
                    if not isinstance(timestamp_raw, str):
                        continue
                    try:
                        parsed = parse_datetime(timestamp_raw)
                        if parsed is None:
                            continue
                        timestamp = unix_ms(parsed)
                    except ValueError:
                        continue
                    if end_ms is not None and timestamp > end_ms:
                        continue
                    values = update.get("values")
                    if isinstance(values, list):
                        yield timestamp, [item for item in values if isinstance(item, dict)]


def collect_samples(
    files: list[Path],
    contexts: list[str],
    start_ms: int | None,
    end_ms: int | None,
    sample_interval_ms: int,
    max_gap_ms: int,
    max_duration_ms: int | None,
) -> list[Sample]:
    state = Sample(timestamp_ms=0)
    samples: list[Sample] = []
    next_sample_ms: int | None = None
    first_sample_ms: int | None = None
    last_update_ms: int | None = None

    for timestamp, values in iter_updates(files, contexts, end_ms):
        recognized_now = 0
        for item in values:
            path = item.get("path")
            if isinstance(path, str) and update_state(state, path, item.get("value")):
                recognized_now += 1
        state.recognized_values += recognized_now

        if start_ms is not None and timestamp < start_ms:
            continue

        if max_duration_ms is not None and first_sample_ms is not None:
            if timestamp - first_sample_ms > max_duration_ms:
                break

        if next_sample_ms is not None and last_update_ms is not None:
            if timestamp - last_update_ms > max_gap_ms:
                next_sample_ms = timestamp
            else:
                while next_sample_ms < timestamp:
                    if state.recognized_values > 0:
                        samples.append(replace(state, timestamp_ms=next_sample_ms))
                        if first_sample_ms is None:
                            first_sample_ms = next_sample_ms
                    next_sample_ms += sample_interval_ms

        if recognized_now == 0:
            last_update_ms = timestamp
            continue

        if next_sample_ms is None:
            next_sample_ms = start_ms if start_ms is not None else timestamp
        while next_sample_ms <= timestamp:
            samples.append(replace(state, timestamp_ms=next_sample_ms))
            if first_sample_ms is None:
                first_sample_ms = next_sample_ms
            next_sample_ms += sample_interval_ms
        last_update_ms = timestamp

    if len(samples) < 2:
        raise RuntimeError("Not enough recognized own-boat data to create a replay")
    return samples


def clamp_int(value: float, minimum: int, maximum: int) -> int:
    if not math.isfinite(value):
        return 0
    return max(minimum, min(maximum, int(round(value))))


def safe_label(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def write_header(output: Path, samples: list[Sample], label: str,
                 timezone_offset_minutes: int) -> None:
    start_ms = samples[0].timestamp_ms
    lines = [
        "#pragma once", "",
        "// Generated by tools/convert_signalk_replay.py. Do not edit by hand.",
        '#include "data/ReplayFormat.h"', "", "namespace elixir::generated {", "",
        f'inline constexpr char kReplayLabel[] = "{safe_label(label)}";',
        f"inline constexpr uint64_t kReplayStartUnixMs = {start_ms}ULL;",
        f"inline constexpr int16_t kReplayTimezoneOffsetMinutes = {timezone_offset_minutes};",
        "", "inline constexpr ReplayRecordPacked kReplayRecords[] = {",
    ]
    for sample in samples:
        values = (
            clamp_int(sample.timestamp_ms - start_ms, 0, 0xFFFFFFFF),
            clamp_int(sample.latitude * 1e7, -2147483648, 2147483647),
            clamp_int(sample.longitude * 1e7, -2147483648, 2147483647),
            clamp_int(sample.sog_kn * 100.0, 0, 65535),
            clamp_int(sample.stw_kn * 100.0, 0, 65535),
            clamp_int(normalize_degrees(sample.cog_deg) * 10.0, 0, 3599),
            clamp_int(normalize_degrees(sample.heading_deg) * 10.0, 0, 3599),
            clamp_int(sample.aws_kn * 100.0, 0, 65535),
            clamp_int(sample.awa_deg * 10.0, -32768, 32767),
            clamp_int(sample.depth_m * 10.0, 0, 65535),
            clamp_int(sample.water_temp_c * 10.0, -32768, 32767),
            clamp_int(sample.roll_deg * 10.0, -32768, 32767),
            clamp_int(sample.pitch_deg * 10.0, -32768, 32767),
            clamp_int(sample.battery_soc_pct * 100.0, 0, 10000),
            clamp_int(sample.battery_voltage_v * 100.0, 0, 65535),
            clamp_int(sample.battery_current_a * 100.0, -32768, 32767),
            clamp_int(sample.solar_power_w, 0, 65535),
            clamp_int(sample.engine_temp_c * 10.0, -32768, 32767),
            clamp_int(sample.alternator_temp_c * 10.0, -32768, 32767),
            clamp_int(sample.engine_room_temp_c * 10.0, -32768, 32767),
        )
        lines.append("    {" + ", ".join(str(item) for item in values) + "},")
    lines.extend([
        "};", "", "inline constexpr ReplayDescriptor kReplayDescriptor{",
        "    kReplayRecords,",
        "    sizeof(kReplayRecords) / sizeof(kReplayRecords[0]),",
        "    kReplayStartUnixMs,", "    kReplayTimezoneOffsetMinutes,",
        "    kReplayLabel,", "};", "", "}  // namespace elixir::generated", "",
    ])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines), encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="Signal K .log/.log.gz files or directories")
    parser.add_argument("--output", default="src/generated/replay_data.h")
    parser.add_argument("--label", default="Signal K replay")
    parser.add_argument("--context", action="append", default=[])
    parser.add_argument("--start", help="ISO-8601 UTC start")
    parser.add_argument("--end", help="ISO-8601 UTC end")
    parser.add_argument("--sample-ms", type=int, default=1000)
    parser.add_argument("--max-gap-seconds", type=int, default=30)
    parser.add_argument("--max-minutes", type=float)
    parser.add_argument("--timezone-offset-min", type=int, default=120)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    files = discover_files(args.inputs)
    if not files:
        print("No .log or .log.gz files found", file=sys.stderr)
        return 2
    contexts = args.context or [
        "ad165619-3cc1-49ba-b7ad-c35d805efb10", "e032a164-cfe5-4cff-a347-2d671d7cc63f",
        "265071450", "vessels.self"
    ]
    start = parse_datetime(args.start)
    end = parse_datetime(args.end)
    max_duration_ms = int(args.max_minutes * 60_000) if args.max_minutes else None
    try:
        samples = collect_samples(
            files, contexts, unix_ms(start) if start else None,
            unix_ms(end) if end else None, max(100, args.sample_ms),
            max(1, args.max_gap_seconds) * 1000, max_duration_ms,
        )
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    output = Path(args.output).expanduser()
    write_header(output, samples, args.label, args.timezone_offset_min)
    duration_s = (samples[-1].timestamp_ms - samples[0].timestamp_ms) / 1000.0
    print(f"Wrote {len(samples):,} records ({duration_s:.1f} s) to {output}",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
