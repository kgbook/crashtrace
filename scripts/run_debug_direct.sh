#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-debug"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug "$@"
cmake --build "${BUILD_DIR}" --target backtrace_debug_direct

if [[ "$(uname -s)" == "Darwin" ]]; then
    dsymutil "${BUILD_DIR}/backtrace_debug_direct" -o "${BUILD_DIR}/backtrace_debug_direct.dSYM"
fi

set +e
"${BUILD_DIR}/backtrace_debug_direct"
STATUS=$?
set -e

echo "debug_direct_exit_code=${STATUS}"

if [[ "${STATUS}" -ne 139 ]]; then
    echo "expected debug direct demo to exit with 139 after SIGSEGV" >&2
    exit 1
fi
