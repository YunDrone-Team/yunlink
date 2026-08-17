#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# shellcheck disable=SC1091
source "${ROOT_DIR}/tools/build_jobs.sh"
yunlink_configure_build_jobs

cmake --preset ninja-debug
cmake --build --preset ninja-debug --parallel "$YUNLINK_BUILD_JOBS" --target \
  yunlink_ffi \
  test_c_ffi_v2
ctest --test-dir "${ROOT_DIR}/build/ninja-debug" -R "test_c_ffi_v2" --output-on-failure

"${ROOT_DIR}/tools/bindings/build_rust_bindings.sh"

if command -v uv >/dev/null 2>&1; then
  uv venv --clear --seed --python 3.12 "${ROOT_DIR}/.venv"
else
  python3 -m venv --clear "${ROOT_DIR}/.venv"
fi
source "${ROOT_DIR}/.venv/bin/activate"
python -m pip install -q --upgrade pip
python -m pip install -q pytest -e "${ROOT_DIR}/bindings/python"
python -m pytest -q "${ROOT_DIR}/bindings/python/tests"

"${ROOT_DIR}/tools/bindings/build_python_wheel.sh"

echo "[bindings-all] OK"
