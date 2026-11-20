#!/usr/bin/env python3
"""Measure TClient's built-in frame intervals in an isolated menu session."""
import argparse
import json
import math
from pathlib import Path
import statistics
import subprocess
import tempfile


def measure(executable, backend, seconds, warmup, extra_commands, required_log, log_output):
    with tempfile.TemporaryDirectory(prefix="axion-benchmark-") as directory:
        work = Path(directory)
        (work / "storage.cfg").write_text(
            "add_path $CURRENTDIR\nadd_path $DATADIR\n", encoding="utf-8"
        )
        frames = work / "frames.txt"
        log = work / "client.log"
        commands = [
            f"logfile {log.as_posix()}",
            f"gfx_backend {backend}",
            "gfx_fullscreen 0",
            "gfx_screen_width 1280",
            "gfx_screen_height 720",
            "gfx_vsync 0",
            "gfx_refresh_rate 0",
            "cl_refresh_rate 0",
            "cl_refresh_rate_inactive 0",
            "cl_show_welcome 0",
            "cl_menu_map jungle",
            *extra_commands,
            f"benchmark_quit {seconds} {frames.as_posix()}",
        ]
        subprocess.run([executable, *commands], cwd=work, timeout=seconds + 60, check=True)
        details = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
        if log_output:
            log_output.write_text(details, encoding="utf-8")
        if not frames.exists():
            raise RuntimeError(f"Client produced no frame data.\n{details[-4000:]}")
        for expected in required_log:
            if expected not in details:
                raise RuntimeError(f"Client log is missing {expected!r}.\n{details[-4000:]}")
        elapsed = 0
        samples = []
        for line in frames.read_text(encoding="ascii").splitlines()[1:]:
            tag, raw, unit = line.split()
            if (tag, unit) != ("Frametime", "us"):
                raise ValueError(f"Unexpected sample: {line}")
            interval = int(raw)
            if interval <= 0:
                continue
            elapsed += interval
            if elapsed >= warmup * 1_000_000:
                samples.append(interval)
        if len(samples) < 100:
            raise RuntimeError(f"Only {len(samples)} positive samples after warmup")
        samples.sort()
        tail_count = math.ceil(len(samples) / 100)
        average = statistics.mean(samples)
        return {
            "frames": len(samples),
            "sample_seconds": sum(samples) / 1_000_000,
            "average_ms": average / 1000,
            "p99_ms": samples[math.ceil(len(samples) * 0.99) - 1] / 1000,
            "average_fps": 1_000_000 / average,
            "one_percent_low_fps": 1_000_000 / statistics.mean(samples[-tail_count:]),
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--backend", default="Vulkan")
    parser.add_argument("--seconds", type=int, default=20)
    parser.add_argument("--warmup", type=int, default=3)
    parser.add_argument("--command", action="append", default=[])
    parser.add_argument("--require-log", action="append", default=[])
    parser.add_argument("--log-output", type=Path)
    args = parser.parse_args()
    if not 0 <= args.warmup < args.seconds:
        parser.error("warmup must be nonnegative and shorter than the run")
    result = measure(
        args.executable.resolve(strict=True), args.backend, args.seconds, args.warmup,
        args.command, args.require_log, args.log_output,
    )
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

