#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/.venv-bindings-wheel"
WHEEL_DIR="${ROOT_DIR}/output/python-wheel"

python3 -m venv "${VENV_DIR}"
source "${VENV_DIR}/bin/activate"
python -m pip install -q --upgrade pip
python -m pip wheel "${ROOT_DIR}/bindings/python" -w "${WHEEL_DIR}"
python -m pip install -q --force-reinstall "${WHEEL_DIR}"/yunlink-*.whl
python -m unittest discover -s "${ROOT_DIR}/bindings/python/tests"

echo "[bindings-python-wheel] OK"
