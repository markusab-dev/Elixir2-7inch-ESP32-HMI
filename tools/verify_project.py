#!/usr/bin/env python3
"""Run all validations available without an ESP32 toolchain or hardware."""
from __future__ import annotations

import csv
import py_compile
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(command: list[str], *, cwd: Path = ROOT) -> None:
    print("+", " ".join(command))
    subprocess.run(command, cwd=cwd, check=True)


def parse_size(value: str) -> int:
    return int(value.strip(), 0)


def verify_partition(path: Path, expected_end: int) -> None:
    highest = 0
    with path.open(newline="", encoding="utf-8") as handle:
        for row in csv.reader(line for line in handle if not line.lstrip().startswith("#")):
            if not row or len(row) < 5:
                continue
            offset = parse_size(row[3])
            size = parse_size(row[4])
            highest = max(highest, offset + size)
    if highest != expected_end:
        raise RuntimeError(
            f"{path.name}: partition end 0x{highest:X}, expected 0x{expected_end:X}"
        )
    print(f"{path.name}: PASS (ends at 0x{highest:X})")


def main() -> int:
    for relative in (
        "tools/convert_signalk_replay.py",
        "tools/generate_demo_replay.py",
        "tests/test_converter.py",
    ):
        py_compile.compile(str(ROOT / relative), doraise=True)
    print("python syntax: PASS")

    verify_partition(ROOT / "partitions_16mb.csv", 0x1000000)
    verify_partition(ROOT / "partitions_8mb.csv", 0x800000)

    if shutil.which("g++") is None:
        raise RuntimeError("g++ is required for host validation")

    with tempfile.TemporaryDirectory(prefix="elixir-verify-") as raw_build:
        build = Path(raw_build)
        common = [
            "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-Iinclude", "-Isrc",
        ]
        run(common + [
            "src/data/DataEngine.cpp",
            "src/data/ReplayDataSource.cpp",
            "src/data/SyntheticDataSource.cpp",
            "src/model/HistoryBuffer.cpp",
            "tests/host_smoke.cpp",
            "-o", str(build / "host_smoke"),
        ])
        run([str(build / "host_smoke")])
        run([sys.executable, "tests/test_converter.py"])

        ui_common = [
            "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-Itests/stubs", "-Iinclude", "-Isrc", "-c",
        ]
        run(ui_common + ["src/ui/UiTheme.cpp", "-o", str(build / "UiTheme.o")])
        run(ui_common + ["src/ui/ElixirUI.cpp", "-o", str(build / "ElixirUI.o")])
        print("ui C++ syntax: PASS")

    generated = (ROOT / "src/generated/replay_data.h").read_text(encoding="utf-8")
    if "kReplayDescriptor" not in generated or "kReplayRecords" not in generated:
        raise RuntimeError("generated replay header is incomplete")
    print("generated replay: PASS")
    print("\nAll host validations passed.")
    print("Full PlatformIO link, flash and hardware tests remain intentionally pending.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
