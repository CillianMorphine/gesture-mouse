#!/usr/bin/env bash
# scripts/rotate_logs.sh
# ─────────────────────────────────────────────────────────────────────────────
# Допоміжний скрипт для зовнішньої ротації логів GestureMouse на Linux.
#
# ПРИМІТКА: Основна ротація за розміром реалізована ВСЕРЕДИНІ програми через
#           spdlog::sinks::rotating_file_sink_mt. Цей скрипт є ДОПОВНЕННЯМ
#           для ротації за ЧАСОМ (добова архівація) та інтеграції з logrotate.
# ─────────────────────────────────────────────────────────────────────────────

LOG_DIR="${1:-./logs}"
ARCHIVE_DIR="${LOG_DIR}/archive"
DAYS_KEEP=30

mkdir -p "$ARCHIVE_DIR"

# Стиснути логи, старіші за 1 день
find "$LOG_DIR" -maxdepth 1 -name "*.log" -mtime +1 | while read -r f; do
    base=$(basename "$f" .log)
    ts=$(date +%Y%m%d_%H%M%S)
    gz_name="${ARCHIVE_DIR}/${base}_${ts}.log.gz"
    gzip -c "$f" > "$gz_name" && rm -f "$f"
    echo "[rotate] Archived: $f → $gz_name"
done

# Видалити архіви, старіші за DAYS_KEEP днів
find "$ARCHIVE_DIR" -name "*.log.gz" -mtime +"$DAYS_KEEP" -delete
echo "[rotate] Cleaned archives older than ${DAYS_KEEP} days"
