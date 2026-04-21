#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    *)
      echo "unknown argument: $1" >&2
      echo "usage: $0 [--build-dir <dir>]" >&2
      exit 2
      ;;
  esac
done

if [[ "${BUILD_DIR}" != /* ]]; then
  BUILD_DIR="${ROOT_DIR}/${BUILD_DIR}"
fi

"${ROOT_DIR}/tools/check_clang_format.sh"
"${ROOT_DIR}/tools/run_clang_tidy.sh" --build-dir "${BUILD_DIR}"
"${ROOT_DIR}/tools/check_core_maxline.sh"

echo "[codestyle] OK"
