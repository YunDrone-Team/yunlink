#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cargo test --workspace --manifest-path "${ROOT_DIR}/bindings/rust/Cargo.toml"

echo "[bindings-rust] OK"
