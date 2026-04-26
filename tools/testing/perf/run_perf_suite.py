#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path


def percentile(values: list[float], pct: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = (len(ordered) - 1) * pct
    lower = math.floor(index)
    upper = math.ceil(index)
    if lower == upper:
        return ordered[lower]
    ratio = index - lower
    return ordered[lower] * (1 - ratio) + ordered[upper] * ratio


def collect_metrics(input_dir: Path) -> dict[str, list[float]]:
    metrics: dict[str, list[float]] = {}
    for path in sorted(input_dir.glob("cases/*.json")):
        payload = json.loads(path.read_text(encoding="utf-8"))
        for key, value in payload.get("metrics", {}).items():
            metrics.setdefault(key, []).append(float(value))
    return metrics


def summarize(metrics: dict[str, list[float]]) -> dict[str, dict[str, float]]:
    summary: dict[str, dict[str, float]] = {}
    for key, values in metrics.items():
        summary[key] = {
            "count": float(len(values)),
            "min": min(values) if values else 0.0,
            "max": max(values) if values else 0.0,
            "p50": percentile(values, 0.50),
            "p95": percentile(values, 0.95),
            "p99": percentile(values, 0.99),
        }
    return summary


def render_markdown(summary: dict[str, dict[str, float]]) -> str:
    lines = ["# Perf Summary", ""]
    for key in sorted(summary):
        stats = summary[key]
        lines.extend(
            [
                f"## {key}",
                "",
                f"- Count: {int(stats['count'])}",
                f"- Min: {stats['min']:.3f}",
                f"- Max: {stats['max']:.3f}",
                f"- P50: {stats['p50']:.3f}",
                f"- P95: {stats['p95']:.3f}",
                f"- P99: {stats['p99']:.3f}",
                "",
            ]
        )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Aggregate perf metrics from testing case JSON files.")
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    input_dir = Path(args.input_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    metrics = collect_metrics(input_dir)
    summary = summarize(metrics)
    (output_dir / "perf-summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    (output_dir / "perf-summary.md").write_text(render_markdown(summary), encoding="utf-8")
    print(f"[perf] wrote {output_dir / 'perf-summary.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
