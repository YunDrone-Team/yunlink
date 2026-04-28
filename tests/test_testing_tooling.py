#!/usr/bin/env python3
"""Regression tests for the new testing scaffolding."""

from __future__ import annotations

import json
import shlex
import socket
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]


class TestingToolingTests(unittest.TestCase):
    def test_dual_host_runner_dry_run_writes_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            out_dir = Path(tmp_dir) / "out"
            result = subprocess.run(
                [
                    "python3",
                    str(ROOT_DIR / "tools" / "testing" / "dual_host" / "run_suite.py"),
                    "--config",
                    str(ROOT_DIR / "tools" / "testing" / "dual_host" / "hosts.example.yaml"),
                    "--suite",
                    "dual-host-baseline",
                    "--output-dir",
                    str(out_dir),
                    "--dry-run",
                ],
                cwd=ROOT_DIR,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            manifest = out_dir / "suite-manifest.json"
            self.assertTrue(manifest.exists())
            payload = json.loads(manifest.read_text(encoding="utf-8"))
            self.assertTrue(payload["dry_run"])
            self.assertEqual(payload["suite"], "dual-host-baseline")
            self.assertTrue(payload["cases"])
            self.assertTrue((out_dir / "summary.json").exists())
            self.assertTrue((out_dir / "summary.md").exists())

    def test_dual_host_runner_executes_local_case(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            out_dir = Path(tmp_dir) / "out"
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.bind(("127.0.0.1", 0))
                port = sock.getsockname()[1]

            air_py = (
                "import socket; "
                "s=socket.socket(); "
                "s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); "
                f"s.bind(('127.0.0.1', {port})); "
                "s.listen(1); "
                "conn,_=s.accept(); "
                "conn.close(); "
                "conn,_=s.accept(); "
                "conn.close(); "
                "s.close()"
            )
            ground_py = (
                "import socket; "
                f"s=socket.create_connection(('127.0.0.1', {port}), timeout=2.0); "
                "s.close()"
            )
            cfg = {
                "hosts": {
                    "host-ground": {
                        "mode": "local",
                        "repo_dir": str(ROOT_DIR),
                        "env": {},
                    },
                    "host-air": {
                        "mode": "local",
                        "repo_dir": str(ROOT_DIR),
                        "env": {},
                    },
                },
                "suites": {
                    "local-exec": {
                        "description": "Local orchestration smoke.",
                        "cases": [
                            {
                                "name": "local-loop",
                                "ground_host": "host-ground",
                                "air_host": "host-air",
                                "air_command": f"{shlex.quote(sys.executable)} -c {shlex.quote(air_py)}",
                                "ground_command": f"{shlex.quote(sys.executable)} -c {shlex.quote(ground_py)}",
                                "air_probe_host": "127.0.0.1",
                                "air_probe_port": port,
                                "required_env": ["local-loopback"],
                                "network_profile": "none",
                                "manual_gate": "nightly-local",
                                "metrics": {"connect_ms": 12.0, "session_ready_ms": 34.0},
                                "artifacts": ["logs/local-loop.txt"],
                                "ground_timeout_s": 3,
                                "air_timeout_s": 3,
                            }
                        ],
                    }
                },
            }
            cfg_path = Path(tmp_dir) / "local-exec.yaml"
            cfg_path.write_text(json.dumps(cfg), encoding="utf-8")

            result = subprocess.run(
                [
                    "python3",
                    str(ROOT_DIR / "tools" / "testing" / "dual_host" / "run_suite.py"),
                    "--config",
                    str(cfg_path),
                    "--suite",
                    "local-exec",
                    "--output-dir",
                    str(out_dir),
                ],
                cwd=ROOT_DIR,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            case_payload = json.loads(
                (out_dir / "cases" / "local-loop.json").read_text(encoding="utf-8")
            )
            self.assertEqual(case_payload["status"], "passed")
            self.assertEqual(case_payload["required_env"], ["local-loopback"])
            self.assertEqual(case_payload["network_profile"], "none")
            self.assertEqual(case_payload["manual_gate"], "nightly-local")
            self.assertEqual(case_payload["metrics"]["connect_ms"], 12.0)
            self.assertEqual(case_payload["artifacts"], ["logs/local-loop.txt"])
            self.assertTrue((out_dir / "summary.json").exists())

    def test_dual_host_runner_executes_multiple_ground_steps(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            out_dir = Path(tmp_dir) / "out"
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.bind(("127.0.0.1", 0))
                port = sock.getsockname()[1]

            air_py = (
                "import socket; "
                "s=socket.socket(); "
                "s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); "
                f"s.bind(('127.0.0.1', {port})); "
                "s.listen(1); "
                "conn,_=s.accept(); conn.close(); "
                "conn,_=s.accept(); conn.close(); "
                "conn,_=s.accept(); conn.close(); "
                "s.close()"
            )
            first_ground_py = (
                "import socket; "
                f"s=socket.create_connection(('127.0.0.1', {port}), timeout=2.0); "
                "s.close()"
            )
            second_ground_py = (
                "import socket; "
                f"s=socket.create_connection(('127.0.0.1', {port}), timeout=2.0); "
                "s.close()"
            )
            cfg = {
                "hosts": {
                    "host-ground": {
                        "mode": "local",
                        "repo_dir": str(ROOT_DIR),
                        "env": {},
                    },
                    "host-air": {
                        "mode": "local",
                        "repo_dir": str(ROOT_DIR),
                        "env": {},
                    },
                },
                "suites": {
                    "local-recovery": {
                        "description": "Local multi-step orchestration smoke.",
                        "cases": [
                            {
                                "name": "local-recovery",
                                "ground_host": "host-ground",
                                "air_host": "host-air",
                                "air_command": f"{shlex.quote(sys.executable)} -c {shlex.quote(air_py)}",
                                "ground_steps": [
                                    {
                                        "name": "ground-1",
                                        "command": f"{shlex.quote(sys.executable)} -c {shlex.quote(first_ground_py)}",
                                        "timeout_s": 3,
                                    },
                                    {
                                        "name": "ground-2",
                                        "command": f"{shlex.quote(sys.executable)} -c {shlex.quote(second_ground_py)}",
                                        "timeout_s": 3,
                                    },
                                ],
                                "air_probe_host": "127.0.0.1",
                                "air_probe_port": port,
                                "air_timeout_s": 3,
                            }
                        ],
                    }
                },
            }
            cfg_path = Path(tmp_dir) / "local-recovery.yaml"
            cfg_path.write_text(json.dumps(cfg), encoding="utf-8")

            result = subprocess.run(
                [
                    "python3",
                    str(ROOT_DIR / "tools" / "testing" / "dual_host" / "run_suite.py"),
                    "--config",
                    str(cfg_path),
                    "--suite",
                    "local-recovery",
                    "--output-dir",
                    str(out_dir),
                ],
                cwd=ROOT_DIR,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            case_payload = json.loads(
                (out_dir / "cases" / "local-recovery.json").read_text(encoding="utf-8")
            )
            self.assertEqual(case_payload["status"], "passed")
            self.assertEqual(case_payload["ground"]["step_count"], 2)
            self.assertEqual(len(case_payload["ground_steps"]), 2)
            self.assertEqual(case_payload["ground_steps"][0]["returncode"], 0)
            self.assertEqual(case_payload["ground_steps"][1]["returncode"], 0)

    def test_report_renderer_writes_summary_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            input_dir = Path(tmp_dir) / "input"
            cases_dir = input_dir / "cases"
            output_dir = Path(tmp_dir) / "output"
            cases_dir.mkdir(parents=True)
            (cases_dir / "a.json").write_text(
                json.dumps({"name": "a", "status": "passed", "duration_s": 1.2}),
                encoding="utf-8",
            )
            (cases_dir / "b.json").write_text(
                json.dumps({"name": "b", "status": "failed", "duration_s": 2.3}),
                encoding="utf-8",
            )
            result = subprocess.run(
                [
                    "python3",
                    str(ROOT_DIR / "tools" / "testing" / "report" / "render_summary.py"),
                    "--input-dir",
                    str(input_dir),
                    "--output-dir",
                    str(output_dir),
                ],
                cwd=ROOT_DIR,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            summary = json.loads((output_dir / "summary.json").read_text(encoding="utf-8"))
            self.assertEqual(summary["total"], 2)
            self.assertEqual(summary["failed"], 1)
            self.assertTrue((output_dir / "summary.md").exists())

    def test_perf_aggregator_writes_percentiles(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            input_dir = Path(tmp_dir) / "input"
            cases_dir = input_dir / "cases"
            output_dir = Path(tmp_dir) / "output"
            cases_dir.mkdir(parents=True)
            metric_rows = [
                {
                    "connect_ms": 10.0,
                    "session_ready_ms": 20.0,
                    "authority_acquire_ms": 30.0,
                    "command_result_ms": 40.0,
                    "state_first_seen_ms": 50.0,
                    "recovery_ms": 60.0,
                },
                {
                    "connect_ms": 20.0,
                    "session_ready_ms": 30.0,
                    "authority_acquire_ms": 40.0,
                    "command_result_ms": 50.0,
                    "state_first_seen_ms": 60.0,
                    "recovery_ms": 70.0,
                },
                {
                    "connect_ms": 30.0,
                    "session_ready_ms": 40.0,
                    "authority_acquire_ms": 50.0,
                    "command_result_ms": 60.0,
                    "state_first_seen_ms": 70.0,
                    "recovery_ms": 80.0,
                },
            ]
            for idx, metrics in enumerate(metric_rows, start=1):
                (cases_dir / f"{idx}.json").write_text(
                    json.dumps(
                        {
                            "name": f"case-{idx}",
                            "metrics": metrics,
                        }
                    ),
                    encoding="utf-8",
                )
            result = subprocess.run(
                [
                    "python3",
                    str(ROOT_DIR / "tools" / "testing" / "perf" / "run_perf_suite.py"),
                    "--input-dir",
                    str(input_dir),
                    "--output-dir",
                    str(output_dir),
                ],
                cwd=ROOT_DIR,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            summary = json.loads((output_dir / "perf-summary.json").read_text(encoding="utf-8"))
            for key, expected_min, expected_max in [
                ("connect_ms", 10.0, 30.0),
                ("session_ready_ms", 20.0, 40.0),
                ("authority_acquire_ms", 30.0, 50.0),
                ("command_result_ms", 40.0, 60.0),
                ("state_first_seen_ms", 50.0, 70.0),
                ("recovery_ms", 60.0, 80.0),
            ]:
                stats = summary[key]
                self.assertEqual(stats["min"], expected_min)
                self.assertEqual(stats["max"], expected_max)
                self.assertGreaterEqual(stats["p95"], stats["p50"])

    def test_netem_scripts_support_dry_run(self) -> None:
        apply_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "netem" / "apply_profile.sh"),
                "--iface",
                "en0",
                "--profile",
                str(ROOT_DIR / "tools" / "testing" / "netem" / "profiles" / "delay.env"),
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        clear_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "netem" / "clear_profile.sh"),
                "--iface",
                "en0",
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(apply_result.returncode, 0, msg=apply_result.stderr or apply_result.stdout)
        self.assertIn("tc", apply_result.stdout)
        self.assertEqual(clear_result.returncode, 0, msg=clear_result.stderr or clear_result.stdout)
        self.assertIn("tc", clear_result.stdout)

    def test_dual_host_helpers_support_dry_run(self) -> None:
        deploy_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "dual_host" / "deploy_artifacts.sh"),
                "--source",
                "build/ninja-debug",
                "--host",
                "local",
                "--dest",
                "/tmp/yunlink-build",
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        collect_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "dual_host" / "collect_logs.sh"),
                "--host",
                "local",
                "--source",
                "/tmp/yunlink-logs",
                "--dest",
                "output/testing/logs",
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        sync_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "dual_host" / "sync_repo.sh"),
                "--host",
                "local",
                "--source",
                str(ROOT_DIR),
                "--dest",
                str(ROOT_DIR / "output" / "testing" / "repo-copy"),
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        bootstrap_result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "testing" / "dual_host" / "bootstrap_host.sh"),
                "--repo-dir",
                str(ROOT_DIR),
                "--with-rust",
                "--dry-run",
            ],
            cwd=ROOT_DIR,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(deploy_result.returncode, 0, msg=deploy_result.stderr or deploy_result.stdout)
        self.assertIn("rsync", deploy_result.stdout)
        self.assertEqual(collect_result.returncode, 0, msg=collect_result.stderr or collect_result.stdout)
        self.assertIn("rsync", collect_result.stdout)
        self.assertEqual(sync_result.returncode, 0, msg=sync_result.stderr or sync_result.stdout)
        self.assertIn("rsync", sync_result.stdout)
        self.assertEqual(
            bootstrap_result.returncode, 0, msg=bootstrap_result.stderr or bootstrap_result.stdout
        )
        self.assertIn("[bootstrap-host]", bootstrap_result.stdout)


if __name__ == "__main__":
    unittest.main()
