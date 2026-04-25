#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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

resolve_clang_tidy() {
  local candidate
  for candidate in \
    "${CLANG_TIDY_BIN:-}" \
    "$(command -v clang-tidy 2>/dev/null || true)" \
    "$(command -v clang-tidy-18 2>/dev/null || true)" \
    "$(command -v clang-tidy-17 2>/dev/null || true)" \
    "$(command -v clang-tidy-16 2>/dev/null || true)" \
    "/opt/homebrew/opt/llvm/bin/clang-tidy" \
    "/opt/homebrew/bin/clang-tidy"; do
    if [[ -n "${candidate}" && -x "${candidate}" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done

  echo "clang-tidy not found. Set CLANG_TIDY_BIN or install LLVM tools." >&2
  return 1
}

if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
  echo "compile_commands.json not found in ${BUILD_DIR}" >&2
  echo "configure the project first: cmake -S . -B ${BUILD_DIR#${ROOT_DIR}/}" >&2
  exit 1
fi

configure_darwin_tidy_build() {
  local llvm_clangxx="/opt/homebrew/opt/llvm/bin/clang++"
  local first_command

  if [[ "$(uname -s)" != "Darwin" || ! -x "${llvm_clangxx}" ]]; then
    return 0
  fi

  first_command="$(
    python3 - "${BUILD_DIR}/compile_commands.json" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as fh:
    data = json.load(fh)

entry = data[0] if data else {}
print(entry.get("command", ""))
PY
  )"

  if [[ "${first_command}" != *"/usr/bin/clang++"* ]]; then
    return 0
  fi

  BUILD_DIR="${BUILD_DIR}-clang-tidy"
  echo "[clang-tidy] detected AppleClang compile database, configuring ${BUILD_DIR}" >&2
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_CXX_COMPILER="${llvm_clangxx}" \
    -DYUNLINK_BUILD_EXAMPLES=ON \
    -DYUNLINK_BUILD_TESTS=ON >/dev/null
}

configure_darwin_tidy_build

list_translation_units() {
  if command -v rg >/dev/null 2>&1; then
    (
      cd "${ROOT_DIR}"
      rg --files src examples tests | rg '\.cpp$'
    )
    return 0
  fi

  (
    cd "${ROOT_DIR}"
    find src examples tests -type f -name '*.cpp' | LC_ALL=C sort
  )
}

CPP_FILES=()
while IFS= read -r file; do
  CPP_FILES+=("${file}")
done < <(list_translation_units)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
  echo "no translation units found under src/examples/tests" >&2
  exit 1
fi

CLANG_TIDY="$(resolve_clang_tidy)"
HEADER_FILTER="^${ROOT_DIR}/(include|src)/"

echo "[clang-tidy] checking ${#CPP_FILES[@]} translation units"
(
  cd "${ROOT_DIR}"
  "${CLANG_TIDY}" \
    -p "${BUILD_DIR}" \
    --quiet \
    --warnings-as-errors='*' \
    --header-filter="${HEADER_FILTER}" \
    "${CPP_FILES[@]}"
)
