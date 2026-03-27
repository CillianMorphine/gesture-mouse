#!/usr/bin/env bash
# =============================================================
# .git/hooks/pre-commit
# Runs clang-format + clang-tidy before every commit.
# If issues are found — commit is BLOCKED.
#
# Install:
#   cp scripts/pre-commit.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit
#
# Skip (emergency):
#   git commit --no-verify
# =============================================================
set -euo pipefail

RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'; NC='\033[0m'
ERRORS=0

echo -e "${GREEN}[pre-commit] Running static analysis...${NC}"

# ── Get only staged .cpp / .h files ──────────────────────────
STAGED=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h|hpp)$' || true)

if [[ -z "$STAGED" ]]; then
    echo -e "${GREEN}[pre-commit] No C++ files staged — skipping lint${NC}"
    exit 0
fi

# ── clang-format check ───────────────────────────────────────
if command -v clang-format &>/dev/null; then
    FORMAT_ISSUES=0
    while IFS= read -r file; do
        [[ -f "$file" ]] || continue
        DIFF=$(clang-format --dry-run --Werror "$file" 2>&1 || true)
        if [[ -n "$DIFF" ]]; then
            echo -e "${YELLOW}  [format] $file needs formatting${NC}"
            FORMAT_ISSUES=$((FORMAT_ISSUES + 1))
        fi
    done <<< "$STAGED"

    if [[ $FORMAT_ISSUES -gt 0 ]]; then
        echo -e "${RED}  ✖ $FORMAT_ISSUES file(s) need clang-format${NC}"
        echo -e "${YELLOW}  Fix with: clang-format -i <file>  OR  ./scripts/lint.sh --format${NC}"
        ERRORS=$((ERRORS + FORMAT_ISSUES))
    else
        echo -e "${GREEN}  ✔ clang-format OK${NC}"
    fi
fi

# ── clang-tidy check ─────────────────────────────────────────
if command -v clang-tidy &>/dev/null && [[ -f "build/compile_commands.json" ]]; then
    TIDY_ISSUES=0
    while IFS= read -r file; do
        [[ -f "$file" && "$file" == src/* ]] || continue
        OUT=$(clang-tidy -p build "$file" 2>&1 || true)
        W=$(echo "$OUT" | grep -c "warning:" || true)
        E=$(echo "$OUT" | grep -c "error:"   || true)
        TOTAL=$((W + E))
        if [[ $TOTAL -gt 0 ]]; then
            echo -e "${YELLOW}  [tidy] $file: $W warnings, $E errors${NC}"
            echo "$OUT" | grep -E "(warning|error):" | head -5
        fi
        TIDY_ISSUES=$((TIDY_ISSUES + TOTAL))
    done <<< "$STAGED"

    if [[ $TIDY_ISSUES -gt 0 ]]; then
        echo -e "${RED}  ✖ clang-tidy: $TIDY_ISSUES issues${NC}"
        ERRORS=$((ERRORS + TIDY_ISSUES))
    else
        echo -e "${GREEN}  ✔ clang-tidy OK${NC}"
    fi
fi

# ── Result ───────────────────────────────────────────────────
if [[ $ERRORS -gt 0 ]]; then
    echo -e "\n${RED}[pre-commit] ✖ COMMIT BLOCKED — $ERRORS issues found${NC}"
    echo -e "${YELLOW}  Fix issues and stage again, or use: git commit --no-verify${NC}"
    exit 1
fi

echo -e "${GREEN}[pre-commit] ✔ All checks passed${NC}"
exit 0
