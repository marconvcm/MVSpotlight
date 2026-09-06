#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "==> Configuring MVSpotlight with CPack DEB..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -DCMAKE_BUILD_TYPE=Release

echo "==> Compiling MVSpotlight..."
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "==> Running test suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> Generating Debian package with CPack..."
cpack --config "${BUILD_DIR}/CPackConfig.cmake" -G DEB

echo "==> Debian package generated in ${BUILD_DIR}:"
ls -la "${BUILD_DIR}"/mvspotlight-*.deb
