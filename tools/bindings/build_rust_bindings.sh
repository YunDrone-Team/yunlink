#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cargo test -p yunlink --manifest-path "${ROOT_DIR}/bindings/rust/Cargo.toml"
cargo build --examples -p yunlink --manifest-path "${ROOT_DIR}/bindings/rust/Cargo.toml"

echo "[bindings-rust] OK"
