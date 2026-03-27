#!/usr/bin/env bash
# =============================================================
# scripts/check_doc_coverage.sh
# Checks documentation coverage: counts undocumented public items
# =============================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'

echo -e "Checking documentation coverage...\n"

TOTAL=0; DOCUMENTED=0; UNDOC_LIST=()

# Count public methods/functions that have Doxygen comments
while IFS= read -r -d '' file; do
    # Count lines with /** or /// (Doxygen comments)
    DOC_COUNT=$(grep -cE '^\s*(/\*\*|///)' "$file" || true)
    # Count public method declarations
    DECL_COUNT=$(grep -cE '^\s+(void|int|float|bool|std::|auto|[[)\s]' "$file" || true)

    if [[ $DECL_COUNT -gt 0 ]]; then
        TOTAL=$((TOTAL + DECL_COUNT))
        DOCUMENTED=$((DOCUMENTED + DOC_COUNT))
        if [[ $DOC_COUNT -lt $DECL_COUNT ]]; then
            UNDOC_LIST+=("$file: ${DOC_COUNT}/${DECL_COUNT} documented")
        fi
    fi
done < <(find include -name "*.h" -print0 2>/dev/null)

# Summary
echo -e "Files scanned: $(find include -name '*.h' | wc -l)"
echo -e "Doc comments:  ${DOCUMENTED}"
echo -e "Declarations:  ${TOTAL}"

if [[ $TOTAL -gt 0 ]]; then
    PCT=$(( DOCUMENTED * 100 / TOTAL ))
    if [[ $PCT -ge 90 ]]; then
        echo -e "Coverage: ${GREEN}${PCT}% ✔${NC}"
    elif [[ $PCT -ge 70 ]]; then
        echo -e "Coverage: ${YELLOW}${PCT}% ⚠${NC}"
    else
        echo -e "Coverage: ${RED}${PCT}% ✖${NC}"
    fi
fi

# Doxygen warnings
if [[ -f "docs/generated/doxygen-warnings.log" ]]; then
    W=$(wc -l < "docs/generated/doxygen-warnings.log")
    echo -e "\nDoxygen warnings: $([ $W -eq 0 ] && echo "${GREEN}0 ✔${NC}" || echo "${YELLOW}${W}${NC}")"
    [[ $W -gt 0 ]] && head -10 docs/generated/doxygen-warnings.log
fi

echo ""
