#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --repo-dir <path> [--preset <name>] [--python-venv <path>] [--with-rust] [--skip-cmake] [--skip-rust-build] [--skip-python-editable] [--dry-run]" >&2
}

REPO_DIR=""
PRESET="ninja-debug"
PYTHON_VENV=".venv"
WITH_RUST=0
SKIP_CMAKE=0
SKIP_RUST_BUILD=0
SKIP_PYTHON_EDITABLE=0
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo-dir) REPO_DIR="$2"; shift 2 ;;
    --preset) PRESET="$2"; shift 2 ;;
    --python-venv) PYTHON_VENV="$2"; shift 2 ;;
    --with-rust) WITH_RUST=1; shift ;;
    --skip-cmake) SKIP_CMAKE=1; shift ;;
    --skip-rust-build) SKIP_RUST_BUILD=1; shift ;;
    --skip-python-editable) SKIP_PYTHON_EDITABLE=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$REPO_DIR" ]]; then
  usage
  exit 2
fi

run_cmd() {
  if [[ "$DRY_RUN" -eq 1 ]]; then
    printf '[bootstrap-host] %q ' "$@"
    printf '\n'
    return 0
  fi
  "$@"
}

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "[bootstrap-host] repo=${REPO_DIR} preset=${PRESET} venv=${PYTHON_VENV} with_rust=${WITH_RUST}"
fi

run_cmd mkdir -p "$REPO_DIR"
cd "$REPO_DIR"

if [[ "$WITH_RUST" -eq 1 && ! -x "${HOME}/.cargo/bin/cargo" ]]; then
  run_cmd bash -lc 'curl https://sh.rustup.rs -sSf | sh -s -- -y --profile minimal'
fi

if [[ "$DRY_RUN" -eq 0 && -f "${HOME}/.cargo/env" ]]; then
  # shellcheck disable=SC1090
  source "${HOME}/.cargo/env"
fi

run_cmd python3 -m venv "$PYTHON_VENV"

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "[bootstrap-host] source ${PYTHON_VENV}/bin/activate"
else
  # shellcheck disable=SC1091
  source "${PYTHON_VENV}/bin/activate"
fi

run_cmd python -m pip install -q --upgrade pip

if [[ "$SKIP_PYTHON_EDITABLE" -eq 0 ]]; then
  run_cmd python -m pip install -q -e bindings/python
fi

if [[ "$SKIP_CMAKE" -eq 0 ]]; then
  run_cmd cmake --preset "$PRESET"
  run_cmd cmake --build "build/${PRESET}" --target \
    example_cxx_air_roundtrip \
    example_cxx_ground_peer_recovery
fi

if [[ "$WITH_RUST" -eq 1 && "$SKIP_RUST_BUILD" -eq 0 ]]; then
  run_cmd cargo build --examples -p yunlink --manifest-path bindings/rust/Cargo.toml
fi

echo "[bootstrap-host] OK"
