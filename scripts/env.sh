#!/usr/bin/env bash

# Script-level environment hooks. Keep toolchain PATH choices out of CMake.
#
# Usage options:
#   CRASHTRACE_PATH_PREPEND="/opt/cross/bin" ./scripts/build_release_symbols.sh
#   cp scripts/local_env.sh.example scripts/local_env.sh
#
# scripts/local_env.sh is intentionally gitignored for machine-specific setup.

CRASHTRACE_SYSTEM_PATH="/usr/bin:/bin:/usr/sbin:/sbin"
if [[ "$(uname -s)" == "Darwin" && -d "/opt/homebrew/bin" ]]; then
    CRASHTRACE_SYSTEM_PATH="${CRASHTRACE_SYSTEM_PATH}:/opt/homebrew/bin"
fi

if [[ -n "${CRASHTRACE_PATH_PREPEND:-}" ]]; then
    export PATH="${CRASHTRACE_PATH_PREPEND}:${CRASHTRACE_SYSTEM_PATH}:${PATH}"
else
    export PATH="${CRASHTRACE_SYSTEM_PATH}:${PATH}"
fi

SCRIPT_ENV_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_ENV_FILE="${SCRIPT_ENV_DIR}/local_env.sh"
if [[ -f "${LOCAL_ENV_FILE}" ]]; then
    # shellcheck source=/dev/null
    source "${LOCAL_ENV_FILE}"
fi
