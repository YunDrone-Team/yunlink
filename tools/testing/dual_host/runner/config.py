from __future__ import annotations

import json
import time
from pathlib import Path

from .models import CommandStep, RemoteCommand


def load_config(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def normalize_case_metadata(case: dict) -> dict:
    metrics = case.get("metrics", {})
    if not isinstance(metrics, dict):
        raise ValueError("case metrics must be an object")
    artifacts = case.get("artifacts", [])
    if not isinstance(artifacts, list):
        raise ValueError("case artifacts must be a list")
    required_env = case.get("required_env", [])
    if not isinstance(required_env, list):
        raise ValueError("case required_env must be a list")
    return {
        "required_env": required_env,
        "network_profile": case.get("network_profile", ""),
        "manual_gate": case.get("manual_gate", ""),
        "metrics": metrics,
        "artifacts": artifacts,
    }


def timestamp_dir() -> str:
    return time.strftime("%Y%m%d-%H%M%S")


def build_remote_command(host_name: str, host_cfg: dict, command: str) -> RemoteCommand:
    return RemoteCommand(
        host_name=host_name,
        mode=host_cfg.get("mode", "ssh"),
        address=host_cfg.get("address", host_name),
        user=host_cfg.get("user", ""),
        repo_dir=host_cfg["repo_dir"],
        command=command,
        env=host_cfg.get("env", {}),
    )


def build_command_steps(
    case: dict,
    host_name: str,
    host_cfg: dict,
    *,
    steps_key: str,
    command_key: str,
    default_timeout_key: str,
    default_step_prefix: str,
) -> list[CommandStep]:
    raw_steps = case.get(steps_key)
    default_timeout_s = float(case.get(default_timeout_key, 15.0))
    if not raw_steps:
        return [
            CommandStep(
                name=f"{default_step_prefix}-1",
                remote=build_remote_command(host_name, host_cfg, case[command_key]),
                timeout_s=default_timeout_s,
            )
        ]

    steps: list[CommandStep] = []
    for index, raw_step in enumerate(raw_steps, start=1):
        if isinstance(raw_step, str):
            step_name = f"{default_step_prefix}-{index}"
            step_command = raw_step
            step_timeout_s = default_timeout_s
        else:
            step_name = raw_step.get("name", f"{default_step_prefix}-{index}")
            step_command = raw_step["command"]
            step_timeout_s = float(raw_step.get("timeout_s", default_timeout_s))
        steps.append(
            CommandStep(
                name=step_name,
                remote=build_remote_command(host_name, host_cfg, step_command),
                timeout_s=step_timeout_s,
            )
        )
    return steps
