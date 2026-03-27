#!/usr/bin/env bash
# =============================================================
# scripts/generate_docs.sh — Generate Doxygen documentation
# Usage:
#   ./scripts/generate_docs.sh           # generate only
#   ./scripts/generate_docs.sh --open    # generate + open browser
#   ./scripts/generate_docs.sh --check   # count warnings only
# =============================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

OPEN_BROWSER=false
CHECK_ONLY=false
OUTPUT_DIR="docs/generated/html"

for arg in "$@"; do
    case "$arg" in
        --open)  OPEN_BROWSER=true ;;
        --check) CHECK_ONLY=true ;;
    esac
done

echo -e "${BOLD}${CYAN}╔══════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║   GestureMouse — Generate Docs       ║${NC}"
echo -e "${BOLD}${CYAN}╚══════════════════════════════════════╝${NC}"

# ── Check Doxygen ─────────────────────────────────────────────
if ! command -v doxygen &>/dev/null; then
    echo -e "${RED}✖ doxygen not found!${NC}"
    echo -e "  Install: sudo apt install doxygen  OR  winget install doxygen.doxygen"
    exit 1
fi

echo -e "\n${GREEN}✔ doxygen $(doxygen --version) found${NC}"

# ── Check Graphviz ────────────────────────────────────────────
if command -v dot &>/dev/null; then
    echo -e "${GREEN}✔ Graphviz (dot) found — diagrams enabled${NC}"
else
    echo -e "${YELLOW}⚠ Graphviz not found — class diagrams disabled${NC}"
    echo -e "  Install: sudo apt install graphviz"
fi

if $CHECK_ONLY; then
    echo -e "\n${BOLD}Checking documentation warnings...${NC}"
    WARNINGS=$(doxygen Doxyfile 2>&1 | grep -c "warning:" || true)
    echo -e "Warnings: ${WARNINGS}"
    [[ $WARNINGS -eq 0 ]] && echo -e "${GREEN}✔ No warnings!${NC}" || echo -e "${YELLOW}⚠ Fix warnings before release${NC}"
    exit 0
fi

# ── Generate ──────────────────────────────────────────────────
echo -e "\n${BOLD}Generating documentation...${NC}"
mkdir -p docs/generated

DOXY_OUTPUT=$(doxygen Doxyfile 2>&1)
WARNINGS=$(echo "$DOXY_OUTPUT" | grep -c "warning:" || true)
ERRORS=$(echo "$DOXY_OUTPUT"   | grep -c "error:"   || true)

echo -e "${DOXY_OUTPUT}" | grep -E "(warning|error):" | head -20

echo -e "\n${BOLD}Results:${NC}"
echo -e "  Warnings: ${YELLOW}${WARNINGS}${NC}"
echo -e "  Errors:   ${RED}${ERRORS}${NC}"
echo -e "  Output:   ${GREEN}${OUTPUT_DIR}/index.html${NC}"

# ── Warning log ───────────────────────────────────────────────
if [[ -f "docs/generated/doxygen-warnings.log" ]]; then
    LOGSIZE=$(wc -l < "docs/generated/doxygen-warnings.log")
    echo -e "  Log:      docs/generated/doxygen-warnings.log (${LOGSIZE} lines)"
fi

# ── Coverage summary ──────────────────────────────────────────
if [[ -d "docs/generated/xml" ]]; then
    TOTAL=$(grep -r "<memberdef" docs/generated/xml/ 2>/dev/null | wc -l || true)
    DOCUMENTED=$(grep -r "briefdescription" docs/generated/xml/ 2>/dev/null | grep -v "<briefdescription/>" | wc -l || true)
    echo -e "\n${BOLD}Coverage:${NC} ~${DOCUMENTED}/${TOTAL} items documented"
fi

echo -e "\n${GREEN}${BOLD}✔ Documentation generated successfully!${NC}"

# ── Open browser ──────────────────────────────────────────────
if $OPEN_BROWSER; then
    INDEX="${OUTPUT_DIR}/index.html"
    if [[ -f "$INDEX" ]]; then
        echo -e "Opening ${INDEX} ..."
        if command -v xdg-open &>/dev/null; then
            xdg-open "$INDEX" &
        elif command -v open &>/dev/null; then
            open "$INDEX"
        fi
    fi
fi
