#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cargo build \
  --manifest-path "${ROOT_DIR}/tools/yunlink_rust_advanced_monitor/Cargo.toml"

if [[ "${1:-}" == "--run" ]]; then
  shift
  cargo run \
    --manifest-path "${ROOT_DIR}/tools/yunlink_rust_advanced_monitor/Cargo.toml" \
    -- "$@"
fi
