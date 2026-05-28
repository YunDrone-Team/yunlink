from __future__ import annotations

import json
import time
from pathlib import Path

from .config import build_command_steps, build_remote_command, normalize_case_metadata
from .models import ProcResult
from .process import ManagedProc, run_once, wait_for_tcp_listener
from .reporting import merge_metrics, write_role_logs


def sanitize_case_name(name: str) -> str:
    cleaned = [ch.lower() if ch.isalnum() else "-" for ch in name.strip()]
    slug = "".join(cleaned).strip("-")
    return slug or "case"


def case_status(air: ProcResult, ground_results: list[ProcResult], error: str) -> str:
    if error or air.returncode != 0 or air.timed_out or not ground_results:
        return "failed"
    for ground in ground_results:
        if ground.returncode != 0 or ground.timed_out:
            return "failed"
    return "passed"


def initial_case_record(case: dict, air, ground_steps: list, dry_run: bool, log_dir: Path) -> dict:
    return {
        "name": case["name"],
        "status": "skipped" if dry_run else "failed",
        "dry_run": dry_run,
        "started_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "air": {"host": air.host_name, "mode": air.mode, "argv": air.rendered()},
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
            "startup_timeout_s": float(case.get("startup_timeout_s", 5.0)),
            "startup_delay_s": float(case.get("startup_delay_s", 0.0)),
        },
        "required_env": [],
        "network_profile": "",
        "manual_gate": "",
        "metrics": {},
        "artifacts": [],
        "generated_artifacts": [],
        "log_dir": str(log_dir),
        "orchestrator_error": "",
    }


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
    case_slug = sanitize_case_name(case["name"])
    case_path = output_dir / "cases" / f"{case_slug}.json"
    log_dir = output_dir / "logs" / case_slug
    record = initial_case_record(case, air, ground_steps, dry_run, log_dir)
    record.update(normalize_case_metadata(case))

    if dry_run:
        case_path.write_text(json.dumps(record, indent=2), encoding="utf-8")
        return record

    air_proc = ManagedProc(air.rendered())
    ground_results: list[ProcResult] = []
    air_result = ProcResult(None, "", "air not started")
    start = time.monotonic()

    try:
        air_proc.start()
        probe = record["probe"]
        if probe["host"] and int(probe["port"]) > 0:
            wait_for_tcp_listener(probe["host"], int(probe["port"]), probe["startup_timeout_s"])
        elif probe["startup_delay_s"] > 0:
            time.sleep(probe["startup_delay_s"])

        for ground_step in ground_steps:
            step_result = run_once(ground_step.remote.rendered(), ground_step.timeout_s)
            ground_results.append(step_result)
            if step_result.returncode != 0 or step_result.timed_out:
                break
        air_result = air_proc.wait(float(case.get("air_timeout_s", 15.0)))
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

    record["duration_s"] = round(time.monotonic() - start, 3)
    record["ended_at"] = time.strftime("%Y-%m-%dT%H:%M:%S%z")
    record["air"].update(air_result.as_dict())
    generated = write_role_logs(log_dir, "air", air_result)
    record["ground"].update(
        (ground_results[-1] if ground_results else ProcResult(None, "", "ground not started"))
        .as_dict()
    )
    for index, result in enumerate(ground_results):
        record["ground_steps"][index].update(result.as_dict())
        generated.extend(write_role_logs(log_dir, ground_steps[index].name, result))
    record["generated_artifacts"] = generated
    metric_texts = [air_result.stdout, air_result.stderr]
    for result in ground_results:
        metric_texts.extend([result.stdout, result.stderr])
    record["metrics"] = merge_metrics(record["metrics"], *metric_texts)
    record["status"] = case_status(air_result, ground_results, record["orchestrator_error"])
    case_path.write_text(json.dumps(record, indent=2), encoding="utf-8")
    return record
