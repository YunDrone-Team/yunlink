from __future__ import annotations

import os
import signal
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass
class ProcResult:
    name: str
    returncode: int | None
    stdout: str
    stderr: str


class ManagedProc:
    def __init__(
        self, name: str, cmd: list[str], cwd: Path, env: dict[str, str] | None = None
    ) -> None:
        self.name = name
        self.cmd = cmd
        self.cwd = cwd
        self.env = env
        self.proc: subprocess.Popen[str] | None = None

    def start(self) -> None:
        self.proc = subprocess.Popen(
            self.cmd,
            cwd=self.cwd,
            env=self.env,
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
        else:
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)

    def kill(self) -> None:
        if not self.proc or self.proc.poll() is not None:
            return
        if os.name == "nt":
            self.proc.kill()
        else:
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def wait(self, timeout: float) -> ProcResult:
        assert self.proc is not None
        try:
            stdout, stderr = self.proc.communicate(timeout=timeout)
            return ProcResult(self.name, self.proc.returncode, stdout, stderr)
        except subprocess.TimeoutExpired:
            return ProcResult(self.name, None, "", f"timeout after {timeout}s")


def print_result(result: ProcResult) -> None:
    print(f"[{result.name}] returncode={result.returncode}")
    if result.stdout.strip():
        print(f"[{result.name}] stdout:\n{result.stdout.strip()}")
    if result.stderr.strip():
        print(f"[{result.name}] stderr:\n{result.stderr.strip()}")


def require_ok(result: ProcResult) -> None:
    print_result(result)
    if result.returncode != 0:
        raise RuntimeError(f"{result.name} failed")


def wait_for_tcp_listener(host: str, port: int, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError(f"listener {host}:{port} not ready in time")
