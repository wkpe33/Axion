"""Measure Axion startup milestones with fresh and reused user data."""
import argparse
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import tempfile


def launch(executable, folder, fast):
    log = folder / "startup.log"
    log.unlink(missing_ok=True)
    frames = folder / "frames.txt"
    commands = [
        f"logfile {log.as_posix()}", "cl_show_welcome 0", "gfx_backend Vulkan",
        "gfx_fullscreen 0", "axion_startup_profile 1", f"axion_fast_startup {fast}",
        f"benchmark_quit 1 {frames.as_posix()}",
    ]
    subprocess.run([executable, *commands], cwd=folder, timeout=45, check=True)
    details = log.read_text(encoding="utf-8", errors="replace")
    stages = {
        key: float(value) for key, value in
        re.findall(r"startup: ([a-z-]+) ([\d.]+) ms", details)
    }
    expected = {"graphics-ready", "first-clear", "initialized", "first-interactive-frame"}
    if expected - stages.keys():
        raise RuntimeError(f"Missing milestones: {expected - stages.keys()}")
    return stages


def setup(folder):
    folder.mkdir()
    (folder / "storage.cfg").write_text("add_path $CURRENTDIR\nadd_path $DATADIR\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--count", type=int, default=10)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    if args.count < 1:
        parser.error("count must be positive")
    results = {}
    for case in ("fresh", "warm"):
        with tempfile.TemporaryDirectory(prefix="axion-startup-") as directory:
            root = Path(directory)
            for fast in (0, 1):
                key = f"{case}-{fast}"
                trials = []
                for index in range(args.count):
                    folder = root / f"{fast}-{index if case == 'fresh' else 'warm'}"
                    if not folder.exists():
                        setup(folder)
                        if case == "warm":
                            launch(executable, folder, fast)
                    sample = launch(executable, folder, fast)
                    trials.append(sample)
                    print(key, index + 1, sample["first-interactive-frame"], flush=True)
                summary = {}
                for stage in trials[0]:
                    samples = sorted(trial[stage] for trial in trials)
                    summary[stage] = {
                        "median_ms": statistics.median(samples),
                        "p95_ms": samples[math.ceil(len(samples) * .95) - 1],
                    }
                results[key] = {"trials": trials, "summary": summary}
                args.output.write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
