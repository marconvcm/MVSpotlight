#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "==> Configuring MVSpotlight with CPack RPM..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -DCMAKE_BUILD_TYPE=Release

echo "==> Compiling MVSpotlight..."
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "==> Running test suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> Generating RPM package with CPack..."
cpack --config "${BUILD_DIR}/CPackConfig.cmake" -G RPM

echo "==> RPM package generated successfully:"
ls -la "${ROOT_DIR}"/mvspotlight-*.rpm
