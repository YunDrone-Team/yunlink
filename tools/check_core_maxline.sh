#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="/Users/groove/.codex/skills/check-maxline/scripts/check_maxline.py"

python3 "${CHECKER}" --root "${ROOT_DIR}/include/sunraycom" --config "${ROOT_DIR}/.codex/check-maxline.json"
python3 "${CHECKER}" --root "${ROOT_DIR}/src" --config "${ROOT_DIR}/.codex/check-maxline.json"
