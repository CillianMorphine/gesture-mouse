#!/usr/bin/env bash
# =============================================================
# docs/scripts/prod.sh — Production process management
#
# Usage:
#   ./docs/scripts/prod.sh start    # build Release + install + start
#   ./docs/scripts/prod.sh stop     # stop the process
#   ./docs/scripts/prod.sh restart  # stop + start
#   ./docs/scripts/prod.sh status   # show status + logs tail
#   ./docs/scripts/prod.sh install  # build Release + install to /usr/local/bin
#   ./docs/scripts/prod.sh logs     # tail live logs
# =============================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

BINARY_NAME="gesture_mouse"
INSTALL_PREFIX="/usr/local"
BUILD_DIR="build"
LOG_FILE="${HOME}/gesture_mouse.log"
PID_FILE="/tmp/gesture_mouse.pid"
SYSTEMD_SERVICE="gesture-mouse"

COMMAND="${1:-help}"

# ── Helpers ───────────────────────────────────────────────────
is_running() {
    if systemctl --user is-active --quiet "${SYSTEMD_SERVICE}" 2>/dev/null; then
        return 0
    fi
    if [ -f "${PID_FILE}" ]; then
        local pid
        pid=$(cat "${PID_FILE}")
        kill -0 "$pid" 2>/dev/null && return 0
    fi
    pgrep -x "${BINARY_NAME}" > /dev/null 2>&1 && return 0
    return 1
}

get_pid() {
    pgrep -x "${BINARY_NAME}" 2>/dev/null | head -1 || echo "N/A"
}

# ── Commands ──────────────────────────────────────────────────
cmd_install() {
    echo -e "${BOLD}Building Release...${NC}"
    cmake -DCMAKE_BUILD_TYPE=Release -B "${BUILD_DIR}" -Wno-dev
    cmake --build "${BUILD_DIR}" -j"$(nproc)"

    echo -e "${BOLD}Installing to ${INSTALL_PREFIX}...${NC}"
    sudo cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

    # Deploy default config if not exists
    if [ ! -f "${HOME}/.config/gesture-mouse/settings.txt" ]; then
        mkdir -p "${HOME}/.config/gesture-mouse"
        cp assets/default_config.txt "${HOME}/.config/gesture-mouse/settings.txt"
        echo -e "  ${GREEN}✔${NC} Default config deployed"
    fi

    echo -e "  ${GREEN}✔${NC} Installed: ${INSTALL_PREFIX}/bin/${BINARY_NAME}"
    "${BINARY_NAME}" --version
}

cmd_start() {
    if is_running; then
        echo -e "${YELLOW}GestureMouse is already running (PID: $(get_pid))${NC}"
        return 0
    fi

    echo -e "${BOLD}Starting GestureMouse (production)...${NC}"

    if systemctl --user list-unit-files "${SYSTEMD_SERVICE}.service" &>/dev/null; then
        systemctl --user start "${SYSTEMD_SERVICE}"
        sleep 1
        systemctl --user status "${SYSTEMD_SERVICE}" --no-pager | head -5
    else
        # Fallback: запуск як фоновий процес
        nohup "${INSTALL_PREFIX}/bin/${BINARY_NAME}" \
            >> "${LOG_FILE}" 2>&1 &
        echo $! > "${PID_FILE}"
        sleep 1
    fi

    if is_running; then
        echo -e "${GREEN}✔ GestureMouse started (PID: $(get_pid))${NC}"
        echo -e "  Log: ${LOG_FILE}"
    else
        echo -e "${RED}✖ GestureMouse failed to start${NC}"
        echo -e "  Check logs: cat ${LOG_FILE}"
        exit 1
    fi
}

cmd_stop() {
    if ! is_running; then
        echo -e "${YELLOW}GestureMouse is not running${NC}"
        return 0
    fi

    echo -e "${BOLD}Stopping GestureMouse...${NC}"

    if systemctl --user is-active --quiet "${SYSTEMD_SERVICE}" 2>/dev/null; then
        systemctl --user stop "${SYSTEMD_SERVICE}"
    else
        pkill -SIGTERM "${BINARY_NAME}" 2>/dev/null || true
        sleep 2
        pkill -SIGKILL "${BINARY_NAME}" 2>/dev/null || true
        rm -f "${PID_FILE}"
    fi

    echo -e "${GREEN}✔ GestureMouse stopped${NC}"
}

cmd_restart() {
    cmd_stop
    sleep 1
    cmd_start
}

cmd_status() {
    echo -e "${BOLD}${CYAN}GestureMouse Status${NC}"
    echo "────────────────────────"

    if is_running; then
        echo -e "  Status:  ${GREEN}RUNNING${NC}"
        echo -e "  PID:     $(get_pid)"
    else
        echo -e "  Status:  ${RED}STOPPED${NC}"
    fi

    echo -e "  Binary:  $(which ${BINARY_NAME} 2>/dev/null || echo 'not found')"
    echo -e "  Version: $("${BINARY_NAME}" --version 2>/dev/null || echo 'N/A')"
    echo -e "  Log:     ${LOG_FILE}"
    echo ""

    if [ -f "${LOG_FILE}" ]; then
        echo -e "${BOLD}Last 10 log lines:${NC}"
        tail -10 "${LOG_FILE}"
    fi
}

cmd_logs() {
    echo -e "${BOLD}Live logs (Ctrl+C to stop):${NC}"
    tail -f "${LOG_FILE}" 2>/dev/null || echo "Log file not found: ${LOG_FILE}"
}

# ── Dispatch ──────────────────────────────────────────────────
case "$COMMAND" in
    install)  cmd_install ;;
    start)    cmd_start ;;
    stop)     cmd_stop ;;
    restart)  cmd_restart ;;
    status)   cmd_status ;;
    logs)     cmd_logs ;;
    *)
        echo "Usage: $0 {install|start|stop|restart|status|logs}"
        echo ""
        echo "  install   Build Release + install to ${INSTALL_PREFIX}/bin"
        echo "  start     Start GestureMouse in background"
        echo "  stop      Stop GestureMouse"
        echo "  restart   Restart GestureMouse"
        echo "  status    Show status and recent logs"
        echo "  logs      Follow live log output"
        exit 1
        ;;
esac
