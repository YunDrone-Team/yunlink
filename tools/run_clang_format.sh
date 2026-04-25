#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="write"

if [[ "${1:-}" == "--check" ]]; then
  MODE="check"
  shift
fi

resolve_clang_format() {
  local candidate
  for candidate in \
    "${CLANG_FORMAT_BIN:-}" \
    "$(command -v clang-format 2>/dev/null || true)" \
    "$(command -v clang-format-18 2>/dev/null || true)" \
    "$(command -v clang-format-17 2>/dev/null || true)" \
    "$(command -v clang-format-16 2>/dev/null || true)" \
    "/opt/homebrew/opt/llvm/bin/clang-format" \
    "/opt/homebrew/bin/clang-format"; do
    if [[ -n "${candidate}" && -x "${candidate}" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done

  echo "clang-format not found. Set CLANG_FORMAT_BIN or install LLVM tools." >&2
  return 1
}

list_cpp_files() {
  if command -v rg >/dev/null 2>&1; then
    (
      cd "${ROOT_DIR}"
      rg --files include src examples tests | rg '\.(hpp|h|cpp)$'
    )
    return 0
  fi

  (
    cd "${ROOT_DIR}"
    find include src examples tests -type f \( -name '*.hpp' -o -name '*.h' -o -name '*.cpp' \) |
      LC_ALL=C sort
  )
}

CPP_FILES=()
while IFS= read -r file; do
  CPP_FILES+=("${file}")
done < <(list_cpp_files)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
  echo "no C++ files found under include/src/examples/tests" >&2
  exit 1
fi

CLANG_FORMAT="$(resolve_clang_format)"

if [[ "${MODE}" == "check" ]]; then
  echo "[clang-format] checking ${#CPP_FILES[@]} files"
  (
    cd "${ROOT_DIR}"
    "${CLANG_FORMAT}" -style=file --dry-run -Werror "${CPP_FILES[@]}"
  )
else
  echo "[clang-format] formatting ${#CPP_FILES[@]} files"
  (
    cd "${ROOT_DIR}"
    "${CLANG_FORMAT}" -style=file -i "${CPP_FILES[@]}"
  )
fi
