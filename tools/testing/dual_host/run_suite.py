#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from runner.cases import run_case
from runner.config import load_config, timestamp_dir
from runner.reporting import render_summary, write_manifest


ROOT_DIR = Path(__file__).resolve().parents[3]
REPORT_RENDERER = ROOT_DIR / "tools" / "testing" / "report" / "render_summary.py"


def main() -> int:
    parser = argparse.ArgumentParser(description="Dual-host suite runner for yunlink.")
    parser.add_argument(
        "--config", required=True, help="Path to host config (.yaml file with JSON content)."
    )
    parser.add_argument("--suite", required=True, help="Suite name to run.")
    parser.add_argument("--output-dir", help="Explicit output directory.")
    parser.add_argument("--dry-run", action="store_true", help="Render commands only.")
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    config = load_config(config_path)
    suite = config["suites"][args.suite]

    output_dir = (
        Path(args.output_dir).resolve()
        if args.output_dir
        else ROOT_DIR / "output" / "testing" / timestamp_dir() / args.suite
    )
    (output_dir / "cases").mkdir(parents=True, exist_ok=True)

    cases = [run_case(config, case, args.dry_run, output_dir) for case in suite["cases"]]
    manifest = {
        "suite": args.suite,
        "description": suite.get("description", ""),
        "dry_run": args.dry_run,
        "config": str(config_path),
        "cases": cases,
    }

    manifest_path = write_manifest(output_dir, manifest)
    render_summary(output_dir, REPORT_RENDERER)
    print(f"[dual-host] wrote {manifest_path}")

    failed = any(case["status"] == "failed" for case in cases)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
