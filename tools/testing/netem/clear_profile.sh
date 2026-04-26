#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: $0 --iface <name> [--dry-run]" >&2
}

IFACE=""
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --iface) IFACE="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    *) usage; exit 2 ;;
  esac
done

if [[ -z "$IFACE" ]]; then
  usage
  exit 2
fi

CMD=(sudo tc qdisc del dev "$IFACE" root)

if [[ "$DRY_RUN" -eq 1 ]]; then
  printf '[netem-clear] %q ' "${CMD[@]}"
  printf '\n'
  exit 0
fi

"${CMD[@]}"
