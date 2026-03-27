#!/usr/bin/env bash
# =============================================================
# docs/scripts/backup.sh — Automated backup for GestureMouse
#
# Usage:
#   ./docs/scripts/backup.sh              # full backup
#   ./docs/scripts/backup.sh --full       # full backup (explicit)
#   ./docs/scripts/backup.sh --incremental # incremental backup
#   ./docs/scripts/backup.sh --verify     # verify latest backup
#   ./docs/scripts/backup.sh --restore    # interactive restore
#   ./docs/scripts/backup.sh --list       # list all backups
#   ./docs/scripts/backup.sh --cleanup    # remove old backups
# =============================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

# ── Configuration ─────────────────────────────────────────────
BACKUP_ROOT="${HOME}/.gesture-mouse-backups"
BINARY_NAME="gesture_mouse"
CONFIG_DIR="${HOME}/.config/gesture-mouse"
SYSTEM_CONFIG_DIR="/etc/gesture-mouse"
LOG_FILE="./gesture_mouse.log"
LOG_DIR="./logs"

# Retention policy (days)
KEEP_DAILY=7
KEEP_WEEKLY=28
KEEP_MONTHLY=90

COMMAND="${1:---full}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DATE_LABEL=$(date +%Y-%m-%d)

mkdir -p "${BACKUP_ROOT}"

# ── Helpers ───────────────────────────────────────────────────
log()  { echo -e "${GREEN}[$(date +%H:%M:%S)]${NC} $*"; }
warn() { echo -e "${YELLOW}[$(date +%H:%M:%S)] ⚠${NC}  $*"; }
err()  { echo -e "${RED}[$(date +%H:%M:%S)] ✖${NC}  $*"; }

verify_archive() {
    local file="$1"
    tar -tzf "$file" > /dev/null 2>&1 && return 0 || return 1
}

create_checksum() {
    local file="$1"
    sha256sum "$file" > "${file}.sha256"
    log "Checksum: ${file}.sha256"
}

# ── Full backup ───────────────────────────────────────────────
do_full_backup() {
    local backup_name="full-${TIMESTAMP}"
    local backup_dir="${BACKUP_ROOT}/${backup_name}"
    local archive="${BACKUP_ROOT}/backup-full-${TIMESTAMP}.tar.gz"

    log "Starting FULL backup..."
    mkdir -p "${backup_dir}"

    # User config
    if [ -d "${CONFIG_DIR}" ]; then
        cp -r "${CONFIG_DIR}" "${backup_dir}/user-config"
        log "Config backed up: ${CONFIG_DIR}"
    else
        warn "User config not found: ${CONFIG_DIR}"
    fi

    # System config
    if [ -d "${SYSTEM_CONFIG_DIR}" ]; then
        sudo cp -r "${SYSTEM_CONFIG_DIR}" "${backup_dir}/system-config" 2>/dev/null || true
        log "System config backed up: ${SYSTEM_CONFIG_DIR}"
    fi

    # Binary
    BINARY_PATH=$(which "${BINARY_NAME}" 2>/dev/null || true)
    if [ -n "${BINARY_PATH}" ]; then
        cp "${BINARY_PATH}" "${backup_dir}/gesture_mouse_binary"
        VERSION=$("${BINARY_NAME}" --version 2>/dev/null || echo "unknown")
        echo "${VERSION}" > "${backup_dir}/version.txt"
        log "Binary backed up: ${BINARY_PATH} (${VERSION})"
    fi

    # Log files
    mkdir -p "${backup_dir}/logs"
    [ -f "${LOG_FILE}" ] && cp "${LOG_FILE}" "${backup_dir}/logs/"
    [ -d "${LOG_DIR}" ] && cp -r "${LOG_DIR}/." "${backup_dir}/logs/" 2>/dev/null || true
    log "Logs backed up"

    # Metadata
    cat > "${backup_dir}/backup-info.txt" << EOF
Backup type: full
Created: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Hostname: $(hostname)
User: $(whoami)
OS: $(uname -a)
GestureMouse version: ${VERSION:-unknown}
EOF

    # Create archive
    tar -czf "${archive}" -C "${BACKUP_ROOT}" "${backup_name}"
    rm -rf "${backup_dir}"
    create_checksum "${archive}"

    ARCHIVE_SIZE=$(du -sh "${archive}" | cut -f1)
    log "Full backup complete: ${archive} (${ARCHIVE_SIZE})"
    echo "${archive}"
}

# ── Incremental backup ────────────────────────────────────────
do_incremental_backup() {
    local backup_name="incr-${TIMESTAMP}"
    local backup_dir="${BACKUP_ROOT}/${backup_name}"
    local archive="${BACKUP_ROOT}/backup-incr-${TIMESTAMP}.tar.gz"

    # Find reference point (last full or incremental backup)
    LAST_BACKUP=$(ls -t "${BACKUP_ROOT}/backup-"*.tar.gz 2>/dev/null | head -1 || true)

    if [ -z "${LAST_BACKUP}" ]; then
        warn "No previous backup found — performing full backup instead"
        do_full_backup
        return
    fi

    log "Starting INCREMENTAL backup (since: $(basename ${LAST_BACKUP}))..."
    mkdir -p "${backup_dir}"

    REF_TIME=$(stat -c %Y "${LAST_BACKUP}")

    # Config — only if changed since last backup
    if [ -d "${CONFIG_DIR}" ]; then
        CHANGED_FILES=$(find "${CONFIG_DIR}" -newer "${LAST_BACKUP}" 2>/dev/null)
        if [ -n "${CHANGED_FILES}" ]; then
            mkdir -p "${backup_dir}/user-config"
            echo "${CHANGED_FILES}" | xargs -I{} cp --parents {} "${backup_dir}/"
            log "Config changes backed up"
        else
            log "Config unchanged — skipping"
        fi
    fi

    # Log — only new entries
    if [ -f "${LOG_FILE}" ] && [ "${LOG_FILE}" -nt "${LAST_BACKUP}" ]; then
        mkdir -p "${backup_dir}/logs"
        cp "${LOG_FILE}" "${backup_dir}/logs/"
        log "Logs backed up"
    fi

    cat > "${backup_dir}/backup-info.txt" << EOF
Backup type: incremental
Created: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Based on: $(basename ${LAST_BACKUP})
Hostname: $(hostname)
EOF

    tar -czf "${archive}" -C "${BACKUP_ROOT}" "${backup_name}"
    rm -rf "${backup_dir}"
    create_checksum "${archive}"

    ARCHIVE_SIZE=$(du -sh "${archive}" | cut -f1)
    log "Incremental backup complete: ${archive} (${ARCHIVE_SIZE})"
}

# ── Verify ────────────────────────────────────────────────────
do_verify() {
    LATEST=$(ls -t "${BACKUP_ROOT}/backup-"*.tar.gz 2>/dev/null | head -1 || true)

    if [ -z "${LATEST}" ]; then
        err "No backups found in ${BACKUP_ROOT}"
        exit 1
    fi

    log "Verifying: $(basename ${LATEST})"

    # Archive integrity
    if verify_archive "${LATEST}"; then
        log "Archive integrity: OK"
    else
        err "Archive integrity: FAILED — archive is corrupted!"
        exit 1
    fi

    # Checksum
    if [ -f "${LATEST}.sha256" ]; then
        if sha256sum -c "${LATEST}.sha256" --quiet 2>/dev/null; then
            log "Checksum verification: OK"
        else
            err "Checksum mismatch — file may have been tampered!"
            exit 1
        fi
    else
        warn "No checksum file found: ${LATEST}.sha256"
    fi

    # Content check
    log "Archive contents:"
    tar -tzf "${LATEST}" | head -20

    log "Verification PASSED"
}

# ── Restore (interactive) ─────────────────────────────────────
do_restore() {
    echo -e "\n${BOLD}Available backups:${NC}"
    mapfile -t BACKUPS < <(ls -t "${BACKUP_ROOT}/backup-"*.tar.gz 2>/dev/null)

    if [ ${#BACKUPS[@]} -eq 0 ]; then
        err "No backups found"
        exit 1
    fi

    for i in "${!BACKUPS[@]}"; do
        SIZE=$(du -sh "${BACKUPS[$i]}" | cut -f1)
        echo "  [$((i+1))] $(basename ${BACKUPS[$i]}) (${SIZE})"
    done

    echo -n "Select backup [1-${#BACKUPS[@]}]: "
    read -r SELECTION
    BACKUP_FILE="${BACKUPS[$((SELECTION-1))]}"

    echo -e "\nWhat to restore?"
    echo "  [1] Config only"
    echo "  [2] Binary only"
    echo "  [3] Everything (full restore)"
    echo -n "Select [1-3]: "
    read -r RESTORE_TYPE

    TMPDIR=$(mktemp -d)
    tar -xzf "${BACKUP_FILE}" -C "${TMPDIR}"
    BACKUP_CONTENT=$(ls "${TMPDIR}")

    case "${RESTORE_TYPE}" in
        1)
            if [ -d "${TMPDIR}/${BACKUP_CONTENT}/user-config" ]; then
                mkdir -p "${CONFIG_DIR}"
                cp -r "${TMPDIR}/${BACKUP_CONTENT}/user-config/." "${CONFIG_DIR}/"
                log "Config restored to ${CONFIG_DIR}"
            fi ;;
        2)
            if [ -f "${TMPDIR}/${BACKUP_CONTENT}/gesture_mouse_binary" ]; then
                sudo cp "${TMPDIR}/${BACKUP_CONTENT}/gesture_mouse_binary" /usr/local/bin/${BINARY_NAME}
                sudo chmod +x /usr/local/bin/${BINARY_NAME}
                log "Binary restored"
            fi ;;
        3)
            [ -d "${TMPDIR}/${BACKUP_CONTENT}/user-config" ] && \
                cp -r "${TMPDIR}/${BACKUP_CONTENT}/user-config/." "${CONFIG_DIR}/"
            [ -f "${TMPDIR}/${BACKUP_CONTENT}/gesture_mouse_binary" ] && \
                sudo cp "${TMPDIR}/${BACKUP_CONTENT}/gesture_mouse_binary" /usr/local/bin/${BINARY_NAME} && \
                sudo chmod +x /usr/local/bin/${BINARY_NAME}
            log "Full restore complete" ;;
    esac

    rm -rf "${TMPDIR}"
    log "Restore complete from: $(basename ${BACKUP_FILE})"
}

# ── List backups ──────────────────────────────────────────────
do_list() {
    echo -e "${BOLD}Backups in ${BACKUP_ROOT}:${NC}\n"
    TOTAL=0
    while IFS= read -r -d '' file; do
        SIZE=$(du -sh "$file" | cut -f1)
        DATE=$(stat -c %y "$file" | cut -d' ' -f1)
        echo "  ${DATE}  ${SIZE}  $(basename $file)"
        TOTAL=$((TOTAL + 1))
    done < <(find "${BACKUP_ROOT}" -name "backup-*.tar.gz" -print0 | sort -z -r)
    echo -e "\nTotal: ${TOTAL} backup(s)"
}

# ── Cleanup old backups ───────────────────────────────────────
do_cleanup() {
    log "Cleaning up old backups (policy: daily=${KEEP_DAILY}d, weekly=${KEEP_WEEKLY}d)..."
    REMOVED=0
    while IFS= read -r -d '' file; do
        FILE_AGE=$(( ( $(date +%s) - $(stat -c %Y "$file") ) / 86400 ))
        if [[ "$(basename $file)" == *"full"* && $FILE_AGE -gt $KEEP_WEEKLY ]]; then
            rm -f "$file" "${file}.sha256"
            REMOVED=$((REMOVED + 1))
        elif [[ "$(basename $file)" == *"incr"* && $FILE_AGE -gt $KEEP_DAILY ]]; then
            rm -f "$file" "${file}.sha256"
            REMOVED=$((REMOVED + 1))
        fi
    done < <(find "${BACKUP_ROOT}" -name "backup-*.tar.gz" -print0)
    log "Removed ${REMOVED} old backup(s)"
}

# ── Dispatch ──────────────────────────────────────────────────
case "${COMMAND}" in
    --full)        do_full_backup ;;
    --incremental) do_incremental_backup ;;
    --verify)      do_verify ;;
    --restore)     do_restore ;;
    --list)        do_list ;;
    --cleanup)     do_cleanup ;;
    *)
        echo "Usage: $0 [--full|--incremental|--verify|--restore|--list|--cleanup]"
        exit 1 ;;
esac
