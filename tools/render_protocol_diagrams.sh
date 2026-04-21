#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/docs/diagrams/plantuml/src"
SVG_DIR="${ROOT_DIR}/docs/diagrams/plantuml/svg"

if [[ -n "${PLANTUML_BIN:-}" ]]; then
  PLANTUML="${PLANTUML_BIN}"
elif command -v plantuml >/dev/null 2>&1; then
  PLANTUML="$(command -v plantuml)"
elif [[ -x "/opt/homebrew/bin/plantuml" ]]; then
  PLANTUML="/opt/homebrew/bin/plantuml"
else
  echo "plantuml not found. Set PLANTUML_BIN or install plantuml." >&2
  exit 1
fi

if [[ ! -d "${SRC_DIR}" ]]; then
  echo "source directory not found: ${SRC_DIR}" >&2
  exit 1
fi

mkdir -p "${SVG_DIR}"
rm -f "${SVG_DIR}"/*.svg

shopt -s nullglob
sources=("${SRC_DIR}"/*.puml)
shopt -u nullglob

if [[ ${#sources[@]} -eq 0 ]]; then
  echo "no PlantUML sources found in ${SRC_DIR}" >&2
  exit 1
fi

"${PLANTUML}" \
  --check-before-run \
  --stop-on-error \
  --svg \
  --charset UTF-8 \
  --skinparam backgroundColor=white \
  --output-dir "${SVG_DIR}" \
  "${sources[@]}"

echo "rendered ${#sources[@]} diagrams into ${SVG_DIR}"
