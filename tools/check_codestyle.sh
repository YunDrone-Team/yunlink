#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

CPP_FILES="$(rg --files include src examples tests compat/legacy_sunray | rg '\.(hpp|h|cpp)$')"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 2
fi

echo "[codestyle] checking C++ formatting..."
echo "$CPP_FILES" | xargs clang-format -style=file --dry-run -Werror

if command -v ruff >/dev/null 2>&1; then
  echo "[codestyle] checking Python lint..."
  ruff check .
else
  echo "[codestyle] ruff not found, skip Python lint check"
fi

echo "[codestyle] OK"
