from __future__ import annotations

import shlex
from dataclasses import dataclass


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
