#!/usr/bin/env bash
# =============================================================
# docs/scripts/dev.sh — Start GestureMouse in development mode
#
# Usage:
#   ./docs/scripts/dev.sh           # build Debug + run
#   ./docs/scripts/dev.sh --debug   # run with OpenCV debug window
#   ./docs/scripts/dev.sh --build   # only build, don't run
#   ./docs/scripts/dev.sh --test    # build + run tests
#   ./docs/scripts/dev.sh --lint    # run static analysis
# =============================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

BUILD_DIR="build"
BUILD_TYPE="Debug"
RUN_APP=true
DEBUG_FLAG=""
RUN_TESTS=false
RUN_LINT=false

for arg in "$@"; do
    case "$arg" in
        --debug)  DEBUG_FLAG="--debug" ;;
        --build)  RUN_APP=false ;;
        --test)   RUN_TESTS=true; RUN_APP=false ;;
        --lint)   RUN_LINT=true; RUN_APP=false ;;
        --release) BUILD_TYPE="Release" ;;
    esac
done

echo -e "${BOLD}${CYAN}"
echo "╔════════════════════════════════════════╗"
echo "║  GestureMouse — Development Mode       ║"
echo "╚════════════════════════════════════════╝"
echo -e "${NC}"

# ── Check dependencies ────────────────────────────────────────
echo -e "${BOLD}[1/4] Checking dependencies...${NC}"

check_dep() {
    if command -v "$1" &>/dev/null; then
        echo -e "  ${GREEN}✔${NC} $1"
    else
        echo -e "  ${RED}✖ $1 not found${NC} — install with: $2"
        exit 1
    fi
}

check_dep "cmake"  "sudo apt install cmake"
check_dep "make"   "sudo apt install build-essential"
check_dep "g++"    "sudo apt install build-essential"

# Check OpenCV
if pkg-config --exists opencv4 2>/dev/null; then
    echo -e "  ${GREEN}✔${NC} OpenCV $(pkg-config --modversion opencv4)"
elif pkg-config --exists opencv 2>/dev/null; then
    echo -e "  ${GREEN}✔${NC} OpenCV $(pkg-config --modversion opencv)"
else
    echo -e "  ${YELLOW}⚠${NC}  OpenCV not found via pkg-config"
    echo -e "     Install: sudo apt install libopencv-dev"
fi

# Check camera
if ls /dev/video* &>/dev/null; then
    echo -e "  ${GREEN}✔${NC} Camera: $(ls /dev/video* | head -1)"
else
    echo -e "  ${YELLOW}⚠${NC}  No camera device found at /dev/video*"
fi

# ── Configure ─────────────────────────────────────────────────
echo -e "\n${BOLD}[2/4] Configuring (${BUILD_TYPE})...${NC}"
cmake \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -B "${BUILD_DIR}" \
    -Wno-dev \
    2>&1 | tail -5

echo -e "  ${GREEN}✔${NC} CMake configured"

# ── Build ─────────────────────────────────────────────────────
echo -e "\n${BOLD}[3/4] Building...${NC}"
START_TIME=$(date +%s)

cmake --build "${BUILD_DIR}" -j"$(nproc)" 2>&1
EXIT_CODE=$?
END_TIME=$(date +%s)
BUILD_TIME=$((END_TIME - START_TIME))

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "  ${GREEN}✔${NC} Build successful in ${BUILD_TIME}s"
    echo -e "  Binary: ${BUILD_DIR}/gesture_mouse"
else
    echo -e "  ${RED}✖ Build FAILED${NC}"
    exit 1
fi

# ── Lint ──────────────────────────────────────────────────────
if $RUN_LINT; then
    echo -e "\n${BOLD}[4/4] Running static analysis...${NC}"
    if [ -f "scripts/lint.sh" ]; then
        chmod +x scripts/lint.sh
        ./scripts/lint.sh || true
    else
        echo -e "  ${YELLOW}⚠${NC}  scripts/lint.sh not found"
    fi
    exit 0
fi

# ── Tests ─────────────────────────────────────────────────────
if $RUN_TESTS; then
    echo -e "\n${BOLD}[4/4] Running tests...${NC}"
    cmake --build "${BUILD_DIR}" --target gesture_mouse_tests 2>&1 | tail -3
    cd "${BUILD_DIR}"
    ctest --output-on-failure -V
    cd ..
    exit 0
fi

# ── Run ───────────────────────────────────────────────────────
if $RUN_APP; then
    echo -e "\n${BOLD}[4/4] Starting GestureMouse (${BUILD_TYPE})...${NC}"
    echo -e "  ${YELLOW}Tip: Press Ctrl+C to stop${NC}"
    echo -e "  Log: gesture_mouse.log\n"
    "./${BUILD_DIR}/gesture_mouse" $DEBUG_FLAG
fi
