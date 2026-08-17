#!/usr/bin/env bash

yunlink_cpu_count() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu 2>/dev/null || echo 1
  else
    echo 1
  fi
}

yunlink_configure_build_jobs() {
  local cores
  cores="$(yunlink_cpu_count)"
  if [[ ! "$cores" =~ ^[0-9]+$ || "$cores" -lt 2 ]]; then
    YUNLINK_BUILD_JOBS=1
  else
    YUNLINK_BUILD_JOBS=$((cores - 1))
  fi
  export YUNLINK_BUILD_JOBS
  export MAKEFLAGS="-j${YUNLINK_BUILD_JOBS}"
  export CMAKE_BUILD_PARALLEL_LEVEL="$YUNLINK_BUILD_JOBS"
  export CARGO_BUILD_JOBS="$YUNLINK_BUILD_JOBS"
  printf '[yunlink-build] cores=%s jobs=%s\n' "$cores" "$YUNLINK_BUILD_JOBS"
}
