#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --host <local|ssh-target> --dest <repo-dir> [--source <repo-dir>] [--exclude-file <path>] [--dry-run]" >&2
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_SOURCE="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
DEFAULT_EXCLUDES="${SCRIPT_DIR}/sync-excludes.txt"

HOST=""
SOURCE="${DEFAULT_SOURCE}"
DEST=""
EXCLUDE_FILE="${DEFAULT_EXCLUDES}"
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host) HOST="$2"; shift 2 ;;
    --source) SOURCE="$2"; shift 2 ;;
    --dest) DEST="$2"; shift 2 ;;
    --exclude-file) EXCLUDE_FILE="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$HOST" || -z "$DEST" ]]; then
  usage
  exit 2
fi

SOURCE="$(cd "$SOURCE" && pwd)"
DEST_PARENT="$(dirname "$DEST")"

if [[ "$HOST" == "local" ]]; then
  mkdir -p "$DEST_PARENT"
  CMD=(
    rsync -az --delete
    --exclude-from "$EXCLUDE_FILE"
    "$SOURCE"/
    "$DEST"/
  )
else
  ssh "$HOST" "mkdir -p $(printf '%q' "$DEST_PARENT")"
  CMD=(
    rsync -az --delete
    --exclude-from "$EXCLUDE_FILE"
    "$SOURCE"/
    "$HOST:$DEST"/
  )
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf '[sync-repo] %q ' "${CMD[@]}"
  printf '\n'
  exit 0
fi

"${CMD[@]}"
echo "[sync-repo] synced ${SOURCE} -> ${HOST}:${DEST}"
