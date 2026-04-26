#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --remote-host <ssh-target> --remote-repo-dir <path> [--local-repo-dir <path>] [--preset <name>] [--dry-run]" >&2
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_REPO_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
REMOTE_HOST=""
REMOTE_REPO_DIR=""
PRESET="ninja-debug"
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --remote-host) REMOTE_HOST="$2"; shift 2 ;;
    --remote-repo-dir) REMOTE_REPO_DIR="$2"; shift 2 ;;
    --local-repo-dir) LOCAL_REPO_DIR="$2"; shift 2 ;;
    --preset) PRESET="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$REMOTE_HOST" || -z "$REMOTE_REPO_DIR" ]]; then
  usage
  exit 2
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  "${SCRIPT_DIR}/bootstrap_host.sh" --repo-dir "$LOCAL_REPO_DIR" --preset "$PRESET" --with-rust --dry-run
  "${SCRIPT_DIR}/sync_repo.sh" --host "$REMOTE_HOST" --source "$LOCAL_REPO_DIR" --dest "$REMOTE_REPO_DIR" --dry-run
else
  "${SCRIPT_DIR}/bootstrap_host.sh" --repo-dir "$LOCAL_REPO_DIR" --preset "$PRESET" --with-rust
  "${SCRIPT_DIR}/sync_repo.sh" --host "$REMOTE_HOST" --source "$LOCAL_REPO_DIR" --dest "$REMOTE_REPO_DIR"
fi

REMOTE_CMD="cd $(printf '%q' "$REMOTE_REPO_DIR") && tools/testing/dual_host/bootstrap_host.sh --repo-dir $(printf '%q' "$REMOTE_REPO_DIR") --preset $(printf '%q' "$PRESET") --with-rust"
if [[ "$DRY_RUN" -eq 1 ]]; then
  REMOTE_CMD+=" --dry-run"
fi
ssh "$REMOTE_HOST" "bash -lc $(printf '%q' "$REMOTE_CMD")"

echo "[prepare-hosts] local + ${REMOTE_HOST} ready"
