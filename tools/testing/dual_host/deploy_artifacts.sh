#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --source <path> --host <local|user@host> --dest <path> [--dry-run]" >&2
}

SOURCE=""
HOST=""
DEST=""
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --source) SOURCE="$2"; shift 2 ;;
    --host) HOST="$2"; shift 2 ;;
    --dest) DEST="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$SOURCE" || -z "$HOST" || -z "$DEST" ]]; then
  usage
  exit 2
fi

if [[ "$HOST" == "local" ]]; then
  CMD=(rsync -a "$SOURCE" "$DEST")
else
  CMD=(rsync -a "$SOURCE" "$HOST:$DEST")
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf '[deploy-artifacts] %q ' "${CMD[@]}"
  printf '\n'
  exit 0
fi

"${CMD[@]}"
