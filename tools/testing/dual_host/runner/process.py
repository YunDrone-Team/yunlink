from __future__ import annotations

import os
import signal
import socket
import subprocess
import time

from .models import ProcResult


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


def run_once(argv: list[str], timeout_s: float) -> ProcResult:
    try:
        result = subprocess.run(
            argv, text=True, capture_output=True, check=False, timeout=timeout_s
        )
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
