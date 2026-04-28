#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shlex
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[3]
REPORT_RENDERER = ROOT_DIR / "tools" / "testing" / "report" / "render_summary.py"


@dataclass
class RemoteCommand:
    host_name: str
    mode: str
    address: str
    user: str
    repo_dir: str
    command: str
    env: dict[str, str]

    def rendered(self) -> list[str]:
        env_prefix = " ".join(
            f"{key}={shlex.quote(value)}" for key, value in sorted(self.env.items())
        )
        shell_command = f"cd {shlex.quote(self.repo_dir)} && "
        if env_prefix:
            shell_command += f"{env_prefix} "
        shell_command += self.command
        if self.mode == "local":
            return ["bash", "--noprofile", "--norc", "-lc", shell_command]
        remote = f"{self.user}@{self.address}" if self.user else self.address
        return ["ssh", remote, f"bash -lc {shlex.quote(shell_command)}"]


@dataclass
class CommandStep:
    name: str
    remote: RemoteCommand
    timeout_s: float


@dataclass
class ProcResult:
    returncode: int | None
    stdout: str
    stderr: str
    timed_out: bool = False

    def as_dict(self) -> dict:
        return {
            "returncode": self.returncode,
            "stdout": self.stdout,
            "stderr": self.stderr,
            "timed_out": self.timed_out,
        }


class ManagedProc:
    def __init__(self, argv: list[str]) -> None:
        self.argv = argv
        self.proc: subprocess.Popen[str] | None = None

    def start(self) -> None:
        self.proc = subprocess.Popen(
            self.argv,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid if os.name != "nt" else None,
        )

    def terminate(self) -> None:
        if not self.proc or self.proc.poll() is not None:
            return
        if os.name == "nt":
            self.proc.terminate()
            return
        os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)

    def kill(self) -> None:
        if not self.proc or self.proc.poll() is not None:
            return
        if os.name == "nt":
            self.proc.kill()
            return
        os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def wait(self, timeout_s: float) -> ProcResult:
        assert self.proc is not None
        try:
            stdout, stderr = self.proc.communicate(timeout=timeout_s)
            return ProcResult(self.proc.returncode, stdout, stderr)
        except subprocess.TimeoutExpired:
            return ProcResult(None, "", f"timeout after {timeout_s}s", timed_out=True)


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


def sanitize_case_name(name: str) -> str:
    cleaned = [
        ch.lower() if ch.isalnum() else "-"
        for ch in name.strip()
    ]
    slug = "".join(cleaned).strip("-")
    return slug or "case"


def run_once(argv: list[str], timeout_s: float) -> ProcResult:
    try:
        result = subprocess.run(argv, text=True, capture_output=True, check=False, timeout=timeout_s)
        return ProcResult(result.returncode, result.stdout, result.stderr)
    except subprocess.TimeoutExpired:
        return ProcResult(None, "", f"timeout after {timeout_s}s", timed_out=True)


def wait_for_tcp_listener(host: str, port: int, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.25):
                return
        except OSError:
            time.sleep(0.05)
    raise TimeoutError(f"listener {host}:{port} not ready after {timeout_s}s")


def case_status(air: ProcResult, ground_results: list[ProcResult], orchestrator_error: str) -> str:
    if orchestrator_error:
        return "failed"
    if air.returncode != 0 or air.timed_out:
        return "failed"
    for ground in ground_results:
        if ground.returncode != 0 or ground.timed_out:
            return "failed"
    if not ground_results:
        return "failed"
    return "passed"


def run_case(config: dict, case: dict, dry_run: bool, output_dir: Path) -> dict:
    hosts = config["hosts"]
    air = build_remote_command(case["air_host"], hosts[case["air_host"]], case["air_command"])
    ground_steps = build_command_steps(
        case,
        case["ground_host"],
        hosts[case["ground_host"]],
        steps_key="ground_steps",
        command_key="ground_command",
        default_timeout_key="ground_timeout_s",
        default_step_prefix="ground",
    )
    startup_timeout_s = float(case.get("startup_timeout_s", 5.0))
    air_timeout_s = float(case.get("air_timeout_s", 15.0))
    startup_delay_s = float(case.get("startup_delay_s", 0.0))
    case_slug = sanitize_case_name(case["name"])
    case_path = output_dir / "cases" / f"{case_slug}.json"

    record = {
        "name": case["name"],
        "status": "skipped" if dry_run else "failed",
        "dry_run": dry_run,
        "started_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "air": {
            "host": air.host_name,
            "mode": air.mode,
            "argv": air.rendered(),
        },
        "ground": {
            "host": ground_steps[0].remote.host_name,
            "mode": ground_steps[0].remote.mode,
            "step_count": len(ground_steps),
        },
        "ground_steps": [
            {
                "name": step.name,
                "host": step.remote.host_name,
                "mode": step.remote.mode,
                "argv": step.remote.rendered(),
                "timeout_s": step.timeout_s,
            }
            for step in ground_steps
        ],
        "probe": {
            "host": case.get("air_probe_host", ""),
            "port": case.get("air_probe_port", 0),
            "startup_timeout_s": startup_timeout_s,
            "startup_delay_s": startup_delay_s,
        },
        "required_env": [],
        "network_profile": "",
        "manual_gate": "",
        "metrics": {},
        "artifacts": [],
        "orchestrator_error": "",
    }
    record.update(normalize_case_metadata(case))

    if dry_run:
        case_path.write_text(json.dumps(record, indent=2), encoding="utf-8")
        return record

    start = time.monotonic()
    air_proc = ManagedProc(air.rendered())
    ground_results: list[ProcResult] = []
    air_result = ProcResult(None, "", "air not started")

    try:
        air_proc.start()
        probe_host = case.get("air_probe_host")
        probe_port = int(case.get("air_probe_port", 0) or 0)
        if probe_host and probe_port > 0:
            wait_for_tcp_listener(probe_host, probe_port, startup_timeout_s)
        elif startup_delay_s > 0:
            time.sleep(startup_delay_s)

        for ground_step in ground_steps:
            step_result = run_once(ground_step.remote.rendered(), ground_step.timeout_s)
            ground_results.append(step_result)
            if step_result.returncode != 0 or step_result.timed_out:
                break
        air_result = air_proc.wait(air_timeout_s)
    except Exception as exc:  # noqa: BLE001
        record["orchestrator_error"] = str(exc)
    finally:
        if air_proc.proc and air_proc.proc.poll() is None:
            air_proc.terminate()
            time.sleep(0.1)
            if air_proc.proc.poll() is None:
                air_proc.kill()
            if air_result.returncode is None and not air_result.timed_out:
                air_result = air_proc.wait(1.0)

    duration_s = round(time.monotonic() - start, 3)
    record["duration_s"] = duration_s
    record["ended_at"] = time.strftime("%Y-%m-%dT%H:%M:%S%z")
    record["air"].update(air_result.as_dict())
    if ground_results:
        final_ground_result = ground_results[-1]
        record["ground"].update(final_ground_result.as_dict())
    else:
        record["ground"].update(ProcResult(None, "", "ground not started").as_dict())
    for index, result in enumerate(ground_results):
        record["ground_steps"][index].update(result.as_dict())
    record["status"] = case_status(air_result, ground_results, record["orchestrator_error"])

    case_path.write_text(json.dumps(record, indent=2), encoding="utf-8")
    return record


def write_manifest(output_dir: Path, manifest: dict) -> Path:
    manifest_path = output_dir / "suite-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest_path


def render_summary(output_dir: Path) -> None:
    subprocess.run(
        [sys.executable, str(REPORT_RENDERER), "--input-dir", str(output_dir), "--output-dir", str(output_dir)],
        check=False,
        text=True,
        capture_output=True,
    )


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
    render_summary(output_dir)
    print(f"[dual-host] wrote {manifest_path}")

    failed = any(case["status"] == "failed" for case in cases)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
