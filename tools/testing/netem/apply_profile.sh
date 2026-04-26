#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --iface <name> --profile <env-file> [--dry-run]" >&2
}

IFACE=""
PROFILE=""
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --iface) IFACE="$2"; shift 2 ;;
    --profile) PROFILE="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$IFACE" || -z "$PROFILE" ]]; then
  usage
  exit 2
fi

source "$PROFILE"

CMD=(sudo tc qdisc replace dev "$IFACE" root netem)

if [[ -n "${DELAY_MS:-}" ]]; then
  CMD+=(delay "${DELAY_MS}ms")
  if [[ -n "${JITTER_MS:-}" && "${JITTER_MS:-0}" != "0" ]]; then
    CMD+=("${JITTER_MS}ms")
  fi
fi

if [[ -n "${LOSS_PERCENT:-}" && "${LOSS_PERCENT:-0}" != "0" ]]; then
  CMD+=(loss "${LOSS_PERCENT}%")
fi

if [[ -n "${REORDER_PERCENT:-}" && "${REORDER_PERCENT:-0}" != "0" ]]; then
  CMD+=(reorder "${REORDER_PERCENT}%")
fi

if [[ -n "${DUPLICATION_PERCENT:-}" && "${DUPLICATION_PERCENT:-0}" != "0" ]]; then
  CMD+=(duplicate "${DUPLICATION_PERCENT}%")
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf '[netem-apply] %q ' "${CMD[@]}"
  printf '\n'
  exit 0
fi

"${CMD[@]}"
