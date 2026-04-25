#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cmake --preset ninja-debug
cmake --build --preset ninja-debug --target \
  yunlink_ffi \
  example_cxx_air_roundtrip \
  test_c_ffi_v1 \
  test_c_ffi_contract
ctest --test-dir "${ROOT_DIR}/build/ninja-debug" -R "test_c_ffi_(v1|contract|loader)" --output-on-failure

"${ROOT_DIR}/tools/bindings/build_rust_bindings.sh"

python3 -m venv "${ROOT_DIR}/.venv"
source "${ROOT_DIR}/.venv/bin/activate"
python -m pip install -q --upgrade pip
python -m pip install -q -e "${ROOT_DIR}/bindings/python"
python -m unittest discover -s "${ROOT_DIR}/bindings/python/tests"

python "${ROOT_DIR}/tools/bindings/run_interop_matrix.py"
"${ROOT_DIR}/tools/bindings/build_python_wheel.sh"

echo "[bindings-all] OK"
