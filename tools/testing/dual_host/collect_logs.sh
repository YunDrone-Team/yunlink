#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --host <local|user@host> --source <path> --dest <path> [--dry-run]" >&2
}

HOST=""
SOURCE=""
DEST=""
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host) HOST="$2"; shift 2 ;;
    --source) SOURCE="$2"; shift 2 ;;
    --dest) DEST="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$HOST" || -z "$SOURCE" || -z "$DEST" ]]; then
  usage
  exit 2
fi

if [[ "$HOST" == "local" ]]; then
  CMD=(rsync -a "$SOURCE" "$DEST")
else
  CMD=(rsync -a "$HOST:$SOURCE" "$DEST")
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf '[collect-logs] %q ' "${CMD[@]}"
  printf '\n'
  exit 0
fi

"${CMD[@]}"
