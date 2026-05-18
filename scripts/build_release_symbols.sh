#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
ARTIFACT_DIR="${ROOT_DIR}/artifacts"
RELEASE_DIR="${ARTIFACT_DIR}/release"
SYMBOLS_DIR="${ARTIFACT_DIR}/symbols"
TOOLS_DIR="${ARTIFACT_DIR}/tools"
SDK_DIR="${ARTIFACT_DIR}/sdk"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "${BUILD_DIR}" --target backtrace_collector backtrace_symbolizer

rm -rf "${ARTIFACT_DIR}"
mkdir -p "${RELEASE_DIR}" "${SYMBOLS_DIR}" "${TOOLS_DIR}" "${SDK_DIR}/include"

cp "${BUILD_DIR}/backtrace_collector" "${RELEASE_DIR}/backtrace_collector"
cp "${BUILD_DIR}/backtrace_symbolizer" "${TOOLS_DIR}/backtrace_symbolizer"
cp "${BUILD_DIR}/libcrashtrace.a" "${SDK_DIR}/libcrashtrace.a"
cp -R "${ROOT_DIR}/lib/include/." "${SDK_DIR}/include/"

case "$(uname -s)" in
    Darwin)
        if ! command -v dsymutil >/dev/null 2>&1; then
            echo "dsymutil is required on macOS to create a dSYM bundle" >&2
            exit 1
        fi
        dsymutil "${BUILD_DIR}/backtrace_collector" -o "${SYMBOLS_DIR}/backtrace_collector.dSYM"
        cp "${BUILD_DIR}/backtrace_collector" "${SYMBOLS_DIR}/backtrace_collector.unstripped"
        cp "${BUILD_DIR}/backtrace_collector" "${SYMBOLS_DIR}/backtrace_collector"
        strip -S "${RELEASE_DIR}/backtrace_collector"
        ;;
    Linux)
        if ! command -v objcopy >/dev/null 2>&1; then
            echo "objcopy is required on Linux to split debug symbols" >&2
            exit 1
        fi
        objcopy --only-keep-debug \
            "${BUILD_DIR}/backtrace_collector" \
            "${SYMBOLS_DIR}/backtrace_collector.debug"
        strip --strip-debug --strip-unneeded "${RELEASE_DIR}/backtrace_collector"
        objcopy "--add-gnu-debuglink=${SYMBOLS_DIR}/backtrace_collector.debug" \
            "${RELEASE_DIR}/backtrace_collector"
        ;;
    *)
        cp "${BUILD_DIR}/backtrace_collector" "${SYMBOLS_DIR}/backtrace_collector.unstripped"
        if command -v strip >/dev/null 2>&1; then
            strip "${RELEASE_DIR}/backtrace_collector" || true
        fi
        ;;
esac

cat > "${ARTIFACT_DIR}/manifest.txt" <<EOF
release_binary=${RELEASE_DIR}/backtrace_collector
symbolizer=${TOOLS_DIR}/backtrace_symbolizer
symbols_dir=${SYMBOLS_DIR}
sdk_dir=${SDK_DIR}
EOF

echo "release binary: ${RELEASE_DIR}/backtrace_collector"
echo "symbolizer: ${TOOLS_DIR}/backtrace_symbolizer"
echo "symbols: ${SYMBOLS_DIR}"
echo "sdk: ${SDK_DIR}"
