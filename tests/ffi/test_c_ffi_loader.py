#!/usr/bin/env python3
"""Shared library loader smoke test for yunlink_ffi."""

from __future__ import annotations

import ctypes
import sys
from pathlib import Path


def find_library(build_dir: Path) -> Path:
    candidates = [
        *build_dir.rglob("libyunlink_ffi*.dylib"),
        *build_dir.rglob("libyunlink_ffi*.so"),
        *build_dir.rglob("yunlink_ffi*.dll"),
    ]
    if not candidates:
        raise FileNotFoundError("yunlink_ffi shared library not found")
    return sorted(candidates)[0]


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_c_ffi_loader.py <build-dir>")

    build_dir = Path(sys.argv[1]).resolve()
    lib_path = find_library(build_dir)
    lib = ctypes.CDLL(str(lib_path))

    required_symbols = [
        "yunlink_ffi_abi_version",
        "yunlink_result_name",
        "yunlink_runtime_create",
        "yunlink_runtime_destroy",
        "yunlink_runtime_start",
        "yunlink_runtime_stop",
        "yunlink_peer_connect",
        "yunlink_session_open",
        "yunlink_session_describe",
        "yunlink_authority_request",
        "yunlink_authority_renew",
        "yunlink_authority_release",
        "yunlink_authority_current",
        "yunlink_command_publish_goto",
        "yunlink_runtime_poll_event",
        "yunlink_runtime_poll_command_result",
        "yunlink_runtime_poll_vehicle_core_state",
    ]
    missing = [name for name in required_symbols if not hasattr(lib, name)]
    if missing:
        raise SystemExit(f"missing exported C ABI symbols: {missing}")

    lib.yunlink_ffi_abi_version.restype = ctypes.c_uint32
    abi_version = lib.yunlink_ffi_abi_version()
    if abi_version != 1:
        raise SystemExit(f"unexpected ABI version: {abi_version}")

    lib.yunlink_result_name.argtypes = [ctypes.c_int]
    lib.yunlink_result_name.restype = ctypes.c_char_p
    name = lib.yunlink_result_name(5)
    if name != b"YUNLINK_RESULT_CONNECT_ERROR":
        raise SystemExit(f"unexpected result name: {name!r}")

    print(f"[ffi-loader] loaded {lib_path.name} abi={abi_version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
