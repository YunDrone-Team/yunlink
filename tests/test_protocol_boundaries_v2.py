#!/usr/bin/env python3
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = re.compile(r"\b(sunray|px4|uav|ugv|ros|feature|gimbal|odom)\b", re.IGNORECASE)
GENERIC_PATHS = [
    ROOT / "include/yunlink/yunlink.hpp",
    ROOT / "include/yunlink/c/yunlink_c.h",
    ROOT / "include/yunlink/core/wire_v2.hpp",
    ROOT / "include/yunlink/core/core_messages_v2.hpp",
    ROOT / "include/yunlink/runtime/runtime_v2.hpp",
    ROOT / "include/yunlink/c/yunlink_v2.h",
    ROOT / "src/core/wire_v2.cpp",
    ROOT / "src/core/wire_v2_codec.cpp",
    ROOT / "src/core/core_messages_v2.cpp",
    ROOT / "src/runtime/v2",
    ROOT / "src/c/v2",
    ROOT / "bindings/rust/yunlink/src/v2.rs",
    ROOT / "bindings/rust/yunlink-sys/src/v2.rs",
    ROOT / "bindings/python/src/yunlink/v2.py",
]


def files(path: Path):
    if path.is_dir():
        yield from (item for item in path.rglob("*") if item.is_file())
    else:
        yield path


violations = []
for root in GENERIC_PATHS:
    for path in files(root):
        for line_number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if FORBIDDEN.search(line):
                violations.append(f"{path.relative_to(ROOT)}:{line_number}: {line.strip()}")

cmake = (ROOT / "CMakeLists.txt").read_text()
target_sources = cmake[cmake.index("set(YUNLINK_LIBRARY_SOURCES"):cmake.index("add_library(yunlink STATIC")]
if "profiles/" in target_sources or "protobuf" in target_sources.lower():
    violations.append("YUNLINK_LIBRARY_SOURCES must not depend on profiles or Protobuf")
for legacy in ("src/c/abi/", "src/runtime/command/", "semantic_messages_state"):
    if legacy in target_sources:
        violations.append(f"legacy source remains in the core target: {legacy}")

if violations:
    raise SystemExit("\n".join(violations))
