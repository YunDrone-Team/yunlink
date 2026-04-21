#!/usr/bin/env python3
"""Build the project with a consistent max(1, cpu_count - 1) parallel policy."""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from typing import Sequence


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--preset",
        default="ninja-debug",
        help="CMake build preset to use (default: ninja-debug)",
    )
    parser.add_argument(
        "--target",
        action="append",
        default=[],
        help="Build target name. Repeat to pass multiple targets.",
    )
    return parser.parse_args()


def compute_jobs() -> tuple[int, int]:
    cores = os.cpu_count() or 1
    jobs = max(1, cores - 1)
    return cores, jobs


def format_command(cmd: Sequence[str]) -> str:
    return " ".join(shlex.quote(part) for part in cmd)


def main() -> int:
    args = parse_args()
    cores, jobs = compute_jobs()

    cmd = ["cmake", "--build", "--preset", args.preset, "--parallel", str(jobs)]
    if args.target:
        cmd.extend(["--target", *args.target])

    print(f"[build_fast] cores={cores}")
    print(f"[build_fast] jobs={jobs}")
    print(f"[build_fast] command={format_command(cmd)}")

    completed = subprocess.run(cmd, check=False)
    return completed.returncode


if __name__ == "__main__":
    sys.exit(main())
