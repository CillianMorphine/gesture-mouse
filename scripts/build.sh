#!/usr/bin/env bash
# scripts/build.sh — Швидка збірка для Linux/macOS

set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"   # Release або Debug

echo "=== GestureMouse Build Script ==="
echo "Build type: $BUILD_TYPE"

# Перевірка залежностей
command -v cmake  >/dev/null 2>&1 || { echo "cmake not found!"; exit 1; }
command -v make   >/dev/null 2>&1 || { echo "make not found!"; exit 1; }

# Перевірка OpenCV
pkg-config --exists opencv4 2>/dev/null || {
    echo "WARNING: OpenCV4 not found via pkg-config. Trying opencv..."
    pkg-config --exists opencv 2>/dev/null || echo "WARNING: OpenCV not found at all!"
}

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make -j"$(nproc)"

echo ""
echo "=== Build complete! ==="
echo "Binary: $(pwd)/gesture_mouse"
echo "Run with: ./$BUILD_DIR/gesture_mouse"
