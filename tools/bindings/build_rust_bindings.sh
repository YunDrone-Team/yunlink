#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# shellcheck disable=SC1091
source "${ROOT_DIR}/tools/build_jobs.sh"
yunlink_configure_build_jobs

cargo test --jobs "$YUNLINK_BUILD_JOBS" --workspace \
  --manifest-path "${ROOT_DIR}/bindings/rust/Cargo.toml"

echo "[bindings-rust] OK"
