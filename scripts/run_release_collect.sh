#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
COLLECTOR="${1:-${ROOT_DIR}/artifacts/release/backtrace_collector}"
LOG_PATH="${2:-${ROOT_DIR}/artifacts/crash_frames.log}"

mkdir -p "$(dirname "${LOG_PATH}")"
set +e
"${COLLECTOR}" >"${LOG_PATH}" 2>&1
STATUS=$?
set -e

cat "${LOG_PATH}"
echo "collector_exit_code=${STATUS}"

if [[ "${STATUS}" -ne 139 ]]; then
    echo "expected collector to exit with 139 after SIGSEGV" >&2
    exit 1
fi
