#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="ninja-debug"
MONITOR_BUILD_DIR="${ROOT_DIR}/tools/yunlink_advanced_monitor/build"
QT_PREFIX="${QT_PREFIX:-}"
RUN_AFTER_BUILD=0
CLEAN=0
MONITOR_ARGS=()

usage() {
  cat <<'EOF'
usage: tools/build_advanced_monitor.sh [options] [-- monitor-args...]

Build yunlink and the Qt advanced monitor with one command.

Options:
  --preset <name>             CMake preset for the main yunlink build.
                              Default: ninja-debug
  --monitor-build-dir <dir>   Build directory for the monitor.
                              Default: tools/yunlink_advanced_monitor/build
  --qt-prefix <dir>           Qt install prefix. Defaults to QT_PREFIX, then
                              common Homebrew qt@5 locations when available.
  --run                       Run the monitor after building it.
  --clean                     Remove the monitor build directory before configuring.
  -h, --help                  Show this help.

Examples:
  tools/build_advanced_monitor.sh
  tools/build_advanced_monitor.sh --run
  tools/build_advanced_monitor.sh --run -- --remote-ip=127.0.0.1 --remote-tcp-port=9696
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --preset)
      PRESET="$2"
      shift 2
      ;;
    --monitor-build-dir)
      MONITOR_BUILD_DIR="$2"
      shift 2
      ;;
    --qt-prefix)
      QT_PREFIX="$2"
      shift 2
      ;;
    --run)
      RUN_AFTER_BUILD=1
      shift
      ;;
    --clean)
      CLEAN=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      MONITOR_ARGS=("$@")
      break
      ;;
    *)
      echo "unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ "${MONITOR_BUILD_DIR}" != /* ]]; then
  MONITOR_BUILD_DIR="${ROOT_DIR}/${MONITOR_BUILD_DIR}"
fi

MAIN_BUILD_DIR="${ROOT_DIR}/build/${PRESET}"

qt_prefix_has_cmake_config() {
  local prefix="$1"
  [[ -f "${prefix}/lib/cmake/Qt5/Qt5Config.cmake" ||
     -f "${prefix}/lib/cmake/Qt6/Qt6Config.cmake" ]]
}

if [[ -z "${QT_PREFIX}" ]] && command -v brew >/dev/null 2>&1; then
  for formula in qt@5 qt; do
    candidate="$(brew --prefix "${formula}" 2>/dev/null || true)"
    if [[ -n "${candidate}" ]] && qt_prefix_has_cmake_config "${candidate}"; then
      QT_PREFIX="${candidate}"
      break
    fi
  done
fi
if [[ -z "${QT_PREFIX}" ]]; then
  for candidate in /opt/homebrew/opt/qt@5 /opt/homebrew/opt/qt /usr/local/opt/qt@5 /usr/local/opt/qt; do
    if [[ -d "${candidate}" ]] && qt_prefix_has_cmake_config "${candidate}"; then
      QT_PREFIX="${candidate}"
      break
    fi
  done
fi

echo "[advanced-monitor] root=${ROOT_DIR}"
echo "[advanced-monitor] preset=${PRESET}"
echo "[advanced-monitor] main-build-dir=${MAIN_BUILD_DIR}"
echo "[advanced-monitor] monitor-build-dir=${MONITOR_BUILD_DIR}"
if [[ -n "${QT_PREFIX}" ]]; then
  echo "[advanced-monitor] qt-prefix=${QT_PREFIX}"
fi

cmake --preset "${PRESET}"
cmake --build --preset "${PRESET}" --target yunlink

if [[ "${CLEAN}" -eq 1 ]]; then
  rm -rf "${MONITOR_BUILD_DIR}"
fi

cmake_args=(
  -S "${ROOT_DIR}/tools/yunlink_advanced_monitor"
  -B "${MONITOR_BUILD_DIR}"
  -DYUNLINK_ROOT="${ROOT_DIR}"
  -DYUNLINK_BUILD_DIR="${MAIN_BUILD_DIR}"
)

if [[ -n "${QT_PREFIX}" ]]; then
  cmake_args+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX}")
fi

cmake "${cmake_args[@]}"

cmake --build "${MONITOR_BUILD_DIR}" --parallel

MONITOR_BIN="${MONITOR_BUILD_DIR}/yunlink_advanced_monitor"
echo "[advanced-monitor] built ${MONITOR_BIN}"

if [[ "${RUN_AFTER_BUILD}" -eq 1 ]]; then
  echo "[advanced-monitor] running ${MONITOR_BIN}"
  if [[ ${#MONITOR_ARGS[@]} -gt 0 ]]; then
    exec "${MONITOR_BIN}" "${MONITOR_ARGS[@]}"
  fi
  exec "${MONITOR_BIN}"
fi
