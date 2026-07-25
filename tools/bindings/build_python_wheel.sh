#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv-bindings-wheel"
WHEEL_DIR="${ROOT_DIR}/output/python-wheel"

mkdir -p "${WHEEL_DIR}"
rm -f "${WHEEL_DIR}"/yunlink-*.whl
# CI runners start clean, but developer workspaces can retain a virtualenv
# whose interpreter was removed or upgraded. Recreate its standard-library
# links before building so the wheel check remains deterministic.
python3 -m venv --clear "${VENV_DIR}"
source "${VENV_DIR}/bin/activate"
python -m pip install -q --upgrade pip pytest
python -m pip wheel "${ROOT_DIR}/bindings/python" -w "${WHEEL_DIR}"
YUNLINK_WHEELS=("${WHEEL_DIR}"/yunlink-*.whl)
if [[ ${#YUNLINK_WHEELS[@]} -ne 1 || ! -f "${YUNLINK_WHEELS[0]}" ]]; then
  echo "expected exactly one YunLink wheel, found ${#YUNLINK_WHEELS[@]}" >&2
  exit 1
fi
python -m pip install -q --force-reinstall "${YUNLINK_WHEELS[0]}"
python -m pytest -q "${ROOT_DIR}/bindings/python/tests"

echo "[bindings-python-wheel] OK"
