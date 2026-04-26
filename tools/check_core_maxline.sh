#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${ROOT_DIR}/.codex/check-maxline.json"

run_check() {
  local scan_root="$1"

  python3 - "${scan_root}" "${CONFIG}" <<'PY'
from __future__ import annotations

import json
import os
import sys
from pathlib import Path, PurePosixPath

root = Path(sys.argv[1]).resolve()
config_path = Path(sys.argv[2]).resolve()
config = json.loads(config_path.read_text(encoding="utf-8"))

max_lines = int(config.get("max_lines", 300))
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
exclude_files = set(config.get("exclude_files", []))
exclude_globs = tuple(config.get("exclude_globs", []))

bad: list[tuple[str, int]] = []
checked = 0

for dirpath, dirnames, filenames in os.walk(root, topdown=True):
    dirnames[:] = [name for name in dirnames if name not in exclude_dirs]
    current_dir = Path(dirpath)
    for filename in sorted(filenames):
        if filename.startswith("."):
            continue
        path = current_dir / filename
        ext = path.suffix.lower().lstrip(".")
        if ext not in include_exts:
            continue
        rel_path = path.relative_to(root).as_posix()
        rel_posix = PurePosixPath(rel_path)
        if rel_path in exclude_files or any(rel_posix.match(pattern) for pattern in exclude_globs):
            continue
        checked += 1
        with path.open("r", encoding="utf-8", errors="replace") as handle:
            line_count = sum(1 for _ in handle)
        if line_count > max_lines:
            bad.append((rel_path, line_count))

if bad:
    print(f"FAILED: {len(bad)} file(s) above {max_lines} lines")
    for rel_path, line_count in sorted(bad, key=lambda item: item[1], reverse=True):
        print(f"- {rel_path}: {line_count}")
    raise SystemExit(1)

ext_label = ",".join(sorted(include_exts))
print(f"OK: checked {checked} file(s) under {root} for extensions [{ext_label}], all <= {max_lines} lines")
PY
}

run_check "${ROOT_DIR}/include/yunlink"
run_check "${ROOT_DIR}/src"
