#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
source "${SCRIPT_DIR}/env.sh"
LOG_PATH="${1:-${ROOT_DIR}/artifacts/crash_frames.log}"
SYMBOLIZER="${2:-${ROOT_DIR}/artifacts/tools/backtrace_symbolizer}"

if [[ "$(uname -s)" == "Darwin" ]]; then
    SYMBOLS="${3:-${ROOT_DIR}/artifacts/symbols/backtrace_collector}"
elif [[ -f "${ROOT_DIR}/artifacts/symbols/backtrace_collector.debug" ]]; then
    SYMBOLS="${3:-${ROOT_DIR}/artifacts/symbols/backtrace_collector.debug}"
else
    SYMBOLS="${3:-${ROOT_DIR}/artifacts/symbols/backtrace_collector.unstripped}"
fi

"${SYMBOLIZER}" --symbols "${SYMBOLS}" --frames "${LOG_PATH}"
