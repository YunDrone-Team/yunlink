#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${ROOT_DIR}/.codex/maxline.json"

python3 - "${ROOT_DIR}" "${CONFIG}" <<'PY'
from __future__ import annotations

import json
import os
import sys
from collections import Counter
from pathlib import Path, PurePosixPath

root_dir = Path(sys.argv[1]).resolve()
config_path = Path(sys.argv[2]).resolve()
config = json.loads(config_path.read_text(encoding="utf-8"))

max_lines = int(config.get("max_lines", 300))
max_files_per_dir = int(config.get("max_files_per_dir", 8))
line_overrides = {
    str(path): int(limit) for path, limit in config.get("max_lines_overrides", {}).items()
}
include_exts = {item.lower().lstrip(".") for item in config.get("include_exts", ["py"])}
exclude_dirs = {
    ".git",
    ".hg",
    ".svn",
    ".venv",
    "venv",
    "node_modules",
    "__pycache__",
    ".pytest_cache",
    ".mypy_cache",
    ".ruff_cache",
    "build",
    "dist",
    *config.get("exclude_dirs", []),
}
exclude_globs = tuple(config.get("exclude_globs", []))
scan_roots = [root_dir / item for item in config.get("scan_roots", ["."])]

bad_lines: list[tuple[str, int]] = []
checked = 0
fanout: Counter[str] = Counter()

def excluded(rel_path: str) -> bool:
    rel_posix = PurePosixPath(rel_path)
    return any(rel_posix.match(pattern) for pattern in exclude_globs)

for scan_root in scan_roots:
    if not scan_root.exists():
        continue
    for dirpath, dirnames, filenames in os.walk(scan_root, topdown=True):
        dirnames[:] = [name for name in dirnames if name not in exclude_dirs]
        current_dir = Path(dirpath)
        for filename in sorted(filenames):
            if filename.startswith("."):
                continue
            path = current_dir / filename
            ext = path.suffix.lower().lstrip(".")
            if ext not in include_exts:
                continue
            rel_path = path.relative_to(root_dir).as_posix()
            if excluded(rel_path):
                continue
            checked += 1
            fanout[path.parent.relative_to(root_dir).as_posix()] += 1
            with path.open("r", encoding="utf-8", errors="replace") as handle:
                line_count = sum(1 for _ in handle)
            if line_count > line_overrides.get(rel_path, max_lines):
                bad_lines.append((rel_path, line_count))

bad_fanout = [(path, count) for path, count in fanout.items() if count > max_files_per_dir]

if bad_lines:
    print(f"FAILED: {len(bad_lines)} file(s) above {max_lines} lines")
    for rel_path, line_count in sorted(bad_lines, key=lambda item: item[1], reverse=True):
        print(f"- {rel_path}: {line_count}")

if bad_fanout:
    print(f"FAILED: {len(bad_fanout)} directory(s) above {max_files_per_dir} source files")
    for rel_path, count in sorted(bad_fanout, key=lambda item: item[1], reverse=True):
        print(f"- {rel_path}: {count}")

if bad_lines or bad_fanout:
    raise SystemExit(1)

ext_label = ",".join(sorted(include_exts))
roots_label = ", ".join(path.relative_to(root_dir).as_posix() for path in scan_roots if path.exists())
print(
    f"OK: checked {checked} source file(s) under [{roots_label}], "
    f"extensions [{ext_label}], configured line limits and <= {max_files_per_dir} files/dir"
)
PY
