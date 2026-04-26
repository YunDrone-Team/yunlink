#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path


def load_cases(input_dir: Path) -> list[dict]:
    case_files = sorted(input_dir.glob("cases/*.json"))
    return [json.loads(path.read_text(encoding="utf-8")) for path in case_files]


def build_summary(cases: list[dict]) -> dict:
    total = len(cases)
    passed = sum(1 for case in cases if case.get("status") == "passed")
    failed = sum(1 for case in cases if case.get("status") == "failed")
    skipped = sum(1 for case in cases if case.get("status") == "skipped")
    durations = [float(case.get("duration_s", 0.0)) for case in cases]
    return {
        "total": total,
        "passed": passed,
        "failed": failed,
        "skipped": skipped,
        "duration_s_total": round(sum(durations), 3),
        "failed_cases": [case["name"] for case in cases if case.get("status") == "failed"],
    }


def render_markdown(summary: dict) -> str:
    lines = [
        "# Test Summary",
        "",
        f"- Total: {summary['total']}",
        f"- Passed: {summary['passed']}",
        f"- Failed: {summary['failed']}",
        f"- Skipped: {summary['skipped']}",
        f"- Duration Total (s): {summary['duration_s_total']}",
    ]
    if summary["failed_cases"]:
        lines.append(f"- Failed Cases: {', '.join(summary['failed_cases'])}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Render testing summary artifacts.")
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    input_dir = Path(args.input_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    cases = load_cases(input_dir)
    summary = build_summary(cases)

    (output_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    (output_dir / "summary.md").write_text(render_markdown(summary), encoding="utf-8")
    print(f"[report] wrote {output_dir / 'summary.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
