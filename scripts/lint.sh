#!/usr/bin/env bash
# =============================================================
# scripts/lint.sh — Run all static analysis tools
# Usage:
#   ./scripts/lint.sh           # check only (no fixes)
#   ./scripts/lint.sh --fix     # apply auto-fixes
#   ./scripts/lint.sh --format  # format code with clang-format
# =============================================================
set -euo pipefail

# ── colours ──────────────────────────────────────────────────
RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

# ── config ───────────────────────────────────────────────────
BUILD_DIR="build"
SRC_DIRS=("src" "include")
FIX_MODE=false
FORMAT_MODE=false
ERRORS=0

for arg in "$@"; do
    case "$arg" in
        --fix)    FIX_MODE=true ;;
        --format) FORMAT_MODE=true ;;
    esac
done

echo -e "${BOLD}${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║   GestureMouse — Static Analysis     ║${NC}"
echo -e "${BOLD}${CYAN}╚══════════════════════════════════════╝${NC}"

# ── Step 1: clang-format ─────────────────────────────────────
echo -e "\n${BOLD}[1/3] clang-format${NC}"

if ! command -v clang-format &>/dev/null; then
    echo -e "${YELLOW}⚠  clang-format not found — skipping${NC}"
else
    CHANGED=0
    while IFS= read -r -d '' file; do
        if $FORMAT_MODE; then
            clang-format -i "$file"
            echo -e "  ${GREEN}✔ formatted:${NC} $file"
        else
            DIFF=$(clang-format --dry-run --Werror "$file" 2>&1 || true)
            if [[ -n "$DIFF" ]]; then
                echo -e "  ${YELLOW}⚠ needs formatting:${NC} $file"
                CHANGED=$((CHANGED + 1))
            fi
        fi
    done < <(find "${SRC_DIRS[@]}" -name "*.cpp" -o -name "*.h" -print0 2>/dev/null)

    if [[ $CHANGED -eq 0 && ! $FORMAT_MODE ]]; then
        echo -e "  ${GREEN}✔ All files are correctly formatted${NC}"
    elif [[ $CHANGED -gt 0 ]]; then
        echo -e "  ${YELLOW}→ Run with --format to auto-fix${NC}"
        ERRORS=$((ERRORS + CHANGED))
    fi
fi

# ── Step 2: clang-tidy ───────────────────────────────────────
echo -e "\n${BOLD}[2/3] clang-tidy${NC}"

if ! command -v clang-tidy &>/dev/null; then
    echo -e "${YELLOW}⚠  clang-tidy not found — skipping${NC}"
    echo -e "   Install: sudo apt install clang-tidy  OR  choco install llvm"
else
    if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
        echo -e "${YELLOW}⚠  compile_commands.json not found.${NC}"
        echo -e "   Run: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B $BUILD_DIR"
    else
        FIX_FLAG=""
        $FIX_MODE && FIX_FLAG="--fix"

        TIDY_OUT=$(run-clang-tidy -p "$BUILD_DIR" \
            -header-filter='^(src|include)/.*' \
            $FIX_FLAG 2>&1 || true)

        WARNINGS=$(echo "$TIDY_OUT" | grep -c "warning:" || true)
        TIDY_ERRORS=$(echo "$TIDY_OUT" | grep -c "error:"   || true)

        echo "$TIDY_OUT" | grep -E "(warning|error):" | head -30

        echo -e "\n  Warnings: ${YELLOW}${WARNINGS}${NC}  Errors: ${RED}${TIDY_ERRORS}${NC}"
        ERRORS=$((ERRORS + WARNINGS + TIDY_ERRORS))
    fi
fi

# ── Step 3: cppcheck ─────────────────────────────────────────
echo -e "\n${BOLD}[3/3] cppcheck${NC}"

if ! command -v cppcheck &>/dev/null; then
    echo -e "${YELLOW}⚠  cppcheck not found — skipping${NC}"
    echo -e "   Install: sudo apt install cppcheck  OR  choco install cppcheck"
else
    CPPCHECK_OUT=$(cppcheck \
        --enable=all \
        --std=c++17 \
        --suppress=missingIncludeSystem \
        --suppress=unmatchedSuppression \
        --error-exitcode=0 \
        --template='{file}:{line}: [{severity}] {message} ({id})' \
        -I include \
        src/ 2>&1 || true)

    CPP_ISSUES=$(echo "$CPPCHECK_OUT" | grep -v "^$" | wc -l)
    echo "$CPPCHECK_OUT" | head -30

    echo -e "\n  Issues found: ${YELLOW}${CPP_ISSUES}${NC}"
    ERRORS=$((ERRORS + CPP_ISSUES))
fi

# ── Summary ──────────────────────────────────────────────────
echo -e "\n${BOLD}${CYAN}══════════════════════════════════════${NC}"
if [[ $ERRORS -eq 0 ]]; then
    echo -e "${GREEN}${BOLD}✔ All checks passed — 0 issues${NC}"
    exit 0
else
    echo -e "${RED}${BOLD}✖ Total issues found: ${ERRORS}${NC}"
    echo -e "${YELLOW}  Run with --fix to apply automatic fixes${NC}"
    exit 1
fi
