#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = re.compile(r"\b(sunray|px4|uav|ugv|ros|feature|gimbal|odom)\b", re.IGNORECASE)
GENERIC_PATHS = [
    ROOT / "include/yunlink",
    ROOT / "src/core",
    ROOT / "src/discovery",
    ROOT / "src/runtime/v2",
    ROOT / "src/c/v2",
    ROOT / "bindings/rust/yunlink/src/v2.rs",
    ROOT / "bindings/rust/yunlink-sys/src/v2.rs",
    ROOT / "bindings/python/src/yunlink/v2.py",
]
ROS_FREE_PATHS = GENERIC_PATHS + [ROOT / "profiles"]


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

ros_protocol = re.compile(
    r"(?:\bros1\b|\bros::|\broscpp\b|\broslib\b|[\"']ros\.|source\.ros|message_definition)",
    re.IGNORECASE,
)
for root in ROS_FREE_PATHS:
    for path in files(root):
        for line_number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if ros_protocol.search(line):
                violations.append(
                    f"ROS protocol leaked into {path.relative_to(ROOT)}:{line_number}: {line.strip()}"
                )

cmake = (ROOT / "CMakeLists.txt").read_text()
target_sources = cmake[cmake.index("set(YUNLINK_LIBRARY_SOURCES"):cmake.index("add_library(yunlink STATIC")]
if "profiles/" in target_sources or "protobuf" in target_sources.lower():
    violations.append("YUNLINK_LIBRARY_SOURCES must not depend on profiles or Protobuf")
for legacy in ("src/c/abi/", "src/runtime/command/", "semantic_messages_state"):
    if legacy in target_sources:
        violations.append(f"legacy source remains in the core target: {legacy}")

legacy_paths = (
    "include/yunlink/c/abi",
    "include/yunlink/runtime/command.hpp",
    "include/yunlink/runtime/state.hpp",
    "include/yunlink/core/semantic/state_types.hpp",
    "src/c/abi",
    "src/runtime/command",
    "src/runtime/state",
    "examples",
)
tracked_paths = set(
    subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True).splitlines()
)
for legacy in legacy_paths:
    if any(path == legacy or path.startswith(f"{legacy}/") for path in tracked_paths):
        violations.append(f"Wire v1 public path still exists: {legacy}")

for path in (ROOT / "CMakePresets.json", ROOT / "bindings/rust/yunlink-sys/build.rs"):
    if "YUNLINK_BUILD_EXAMPLES" in path.read_text():
        violations.append(f"removed examples option remains in {path.relative_to(ROOT)}")

if violations:
    raise SystemExit("\n".join(violations))
