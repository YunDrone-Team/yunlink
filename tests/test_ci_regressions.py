#!/usr/bin/env python3
"""Regression tests for CI-only tooling and smoke orchestration bugs."""

from __future__ import annotations

import importlib.util
import json
import os
import socket
import subprocess
import sys
import tempfile
import textwrap
import threading
import time
import unittest
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
SMOKE_SCRIPT = ROOT_DIR / "examples" / "smoke_local" / "run_smoke_local.py"


def load_smoke_module():
    spec = importlib.util.spec_from_file_location("run_smoke_local", SMOKE_SCRIPT)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class SmokeHelperTests(unittest.TestCase):
    def test_wait_for_tcp_listener_handles_delayed_start(self) -> None:
        smoke = load_smoke_module()
        host = "127.0.0.1"

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            probe.bind((host, 0))
            port = probe.getsockname()[1]

        def delayed_server() -> None:
            time.sleep(0.35)
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
                srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                srv.bind((host, port))
                srv.listen(1)
                conn, _ = srv.accept()
                conn.close()

        thread = threading.Thread(target=delayed_server, daemon=True)
        thread.start()

        self.assertTrue(smoke.wait_for_tcp_listener(host, port, timeout_s=2.0))
        thread.join(timeout=1.0)


class ToolingFallbackTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.work_dir = Path(self.temp_dir.name)
        self.fake_bin = self.work_dir / "bin"
        self.fake_bin.mkdir()
        self.log_file = self.work_dir / "tool.log"

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_tool(self, name: str) -> Path:
        path = self.fake_bin / name
        path.write_text(
            textwrap.dedent(
                f"""\
                #!/usr/bin/env bash
                set -euo pipefail
                printf '%s\\n' "$@" >> "{self.log_file}"
                exit 0
                """
            ),
            encoding="utf-8",
        )
        path.chmod(0o755)
        return path

    def base_env(self) -> dict[str, str]:
        path_entries = [str(self.fake_bin), os.path.dirname(sys.executable), "/usr/bin", "/bin"]
        env = os.environ.copy()
        env["PATH"] = os.pathsep.join(path_entries)
        return env

    def test_run_clang_format_works_without_rg(self) -> None:
        fake_clang_format = self.write_tool("fake-clang-format")
        env = self.base_env()
        env["CLANG_FORMAT_BIN"] = str(fake_clang_format)

        result = subprocess.run(
            ["bash", str(ROOT_DIR / "tools" / "run_clang_format.sh"), "--check"],
            cwd=ROOT_DIR,
            env=env,
            text=True,
            capture_output=True,
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
        self.assertIn("[clang-format] checking", result.stdout)
        self.assertTrue(self.log_file.exists(), "fake clang-format was not called")

    def test_run_clang_tidy_works_without_rg(self) -> None:
        fake_clang_tidy = self.write_tool("fake-clang-tidy")
        build_dir = self.work_dir / "build"
        build_dir.mkdir()
        compile_commands = [
            {
                "directory": str(ROOT_DIR),
                "command": "clang++ -Iinclude -c src/core/protocol_codec.cpp",
                "file": str(ROOT_DIR / "src" / "core" / "protocol_codec.cpp"),
            }
        ]
        (build_dir / "compile_commands.json").write_text(
            json.dumps(compile_commands), encoding="utf-8"
        )

        env = self.base_env()
        env["CLANG_TIDY_BIN"] = str(fake_clang_tidy)

        result = subprocess.run(
            [
                "bash",
                str(ROOT_DIR / "tools" / "run_clang_tidy.sh"),
                "--build-dir",
                str(build_dir),
            ],
            cwd=ROOT_DIR,
            env=env,
            text=True,
            capture_output=True,
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
        self.assertIn("[clang-tidy] checking", result.stdout)
        self.assertTrue(self.log_file.exists(), "fake clang-tidy was not called")


if __name__ == "__main__":
    unittest.main()
