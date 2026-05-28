from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from .models import ProcResult

METRICS_PREFIX = "YUNLINK_METRICS "
METRIC_KEYS = (
    "connect_ms",
    "session_ready_ms",
    "authority_acquire_ms",
    "command_result_ms",
    "state_first_seen_ms",
    "recovery_ms",
)


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_role_logs(log_dir: Path, role: str, result: ProcResult) -> list[str]:
    stdout_path = log_dir / f"{role}-stdout.log"
    stderr_path = log_dir / f"{role}-stderr.log"
    write_text(stdout_path, result.stdout)
    write_text(stderr_path, result.stderr)
    return [str(stdout_path), str(stderr_path)]


def extract_metrics(text: str) -> dict[str, float]:
    metrics: dict[str, float] = {}
    for line in text.splitlines():
        if not line.startswith(METRICS_PREFIX):
            continue
        payload = json.loads(line[len(METRICS_PREFIX) :].strip())
        if not isinstance(payload, dict):
            raise ValueError("metrics payload must be an object")
        for key in METRIC_KEYS:
            value = payload.get(key)
            if value is not None:
                metrics[key] = float(value)
    return metrics


def merge_metrics(base: dict, *texts: str) -> dict[str, float]:
    merged = {key: float(base.get(key, 0.0)) for key in METRIC_KEYS}
    for text in texts:
        for key, value in extract_metrics(text).items():
            merged[key] = value
    return merged


def write_manifest(output_dir: Path, manifest: dict) -> Path:
    manifest_path = output_dir / "suite-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest_path


def render_summary(output_dir: Path, report_renderer: Path) -> None:
    subprocess.run(
        [
            sys.executable,
            str(report_renderer),
            "--input-dir",
            str(output_dir),
            "--output-dir",
            str(output_dir),
        ],
        check=False,
        text=True,
        capture_output=True,
    )
