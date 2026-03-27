# Резервне копіювання (Backup & Recovery)

> **Для кого цей документ:** release engineer, DevOps-спеціаліст, системний адміністратор.

---

## Особливості проєкту з точки зору резервного копіювання

GestureMouse є десктопною утилітою без СУБД. Дані для резервного копіювання:

| Тип даних | Розташування | Критичність | Розмір |
|---|---|---|---|
| Конфіг користувача | `~/.config/gesture-mouse/` | Висока | < 1 КБ |
| Системна конфігурація | `/etc/gesture-mouse/` | Середня | < 1 КБ |
| Лог-файли | `./gesture_mouse.log` | Низька | до 50 МБ |
| Бінарний файл | `/usr/local/bin/gesture_mouse` | Висока | ~10 МБ |
| Кастомні моделі (майб.) | `~/.config/gesture-mouse/models/` | Висока | до 100 МБ |

**Що НЕ потрібно резервувати:**
- Вихідний код (зберігається в Git-репозиторії)
- Бінарники з офіційних релізів (можна перезавантажити)
- Тимчасові файли

---

## 1. Стратегія резервного копіювання

### 1.1 Типи резервних копій

| Тип | Частота | Зміст | Розмір |
|---|---|---|---|
| **Повна (full)** | Щотижня (неділя, 03:00) | Все: конфіг + логи + бінарник | ~15 МБ |
| **Інкрементальна** | Щодня (03:00, крім неділі) | Тільки зміни з попередньої копії | < 1 МБ |
| **Перед оновленням** | Перед кожним оновленням | Повна копія поточного стану | ~15 МБ |

### 1.2 Ротація та зберігання

```
Схема ротації (правило 3-2-1):
- 3 копії даних
- 2 різні носії (локальний диск + мережеве сховище/хмара)
- 1 копія offsite (хмара / інший фізичний вузол)

Термін зберігання:
- Щоденні копії:    зберігати 7 днів
- Щотижневі копії:  зберігати 4 тижні
- Місячні копії:    зберігати 3 місяці
```

---

## 2. Процедура резервного копіювання

### 2.1 Ручне резервне копіювання

```bash
#!/bin/bash
# Швидке ручне резервне копіювання

BACKUP_ROOT="$HOME/.gesture-mouse-backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="$BACKUP_ROOT/manual-$TIMESTAMP"

mkdir -p "$BACKUP_DIR"

# Конфігурація користувача
[ -d "$HOME/.config/gesture-mouse" ] && \
    cp -r "$HOME/.config/gesture-mouse" "$BACKUP_DIR/user-config"

# Системна конфігурація
[ -d "/etc/gesture-mouse" ] && \
    sudo cp -r "/etc/gesture-mouse" "$BACKUP_DIR/system-config"

# Бінарний файл
BINARY=$(which gesture_mouse 2>/dev/null)
[ -n "$BINARY" ] && cp "$BINARY" "$BACKUP_DIR/gesture_mouse_binary"

# Лог-файли
find . -name "gesture_mouse*.log" -exec cp {} "$BACKUP_DIR/" \;

# Створити архів
tar -czf "$BACKUP_ROOT/backup-$TIMESTAMP.tar.gz" -C "$BACKUP_ROOT" "manual-$TIMESTAMP"
rm -rf "$BACKUP_DIR"

echo "Backup created: $BACKUP_ROOT/backup-$TIMESTAMP.tar.gz"
echo "Size: $(du -sh $BACKUP_ROOT/backup-$TIMESTAMP.tar.gz | cut -f1)"
```

### 2.2 Перевірка цілісності копій

```bash
# Перевірити цілісність архіву
BACKUP_FILE="$HOME/.gesture-mouse-backups/backup-TIMESTAMP.tar.gz"

# Метод 1: перевірка tar
tar -tzf "$BACKUP_FILE" > /dev/null && echo "OK: Archive is valid" || echo "ERROR: Archive is corrupted"

# Метод 2: SHA256-контрольна сума
sha256sum "$BACKUP_FILE" > "$BACKUP_FILE.sha256"
sha256sum -c "$BACKUP_FILE.sha256" && echo "OK: Checksum valid" || echo "ERROR: Checksum mismatch"

# Метод 3: тестове розпакування у тимчасову директорію
TMPDIR=$(mktemp -d)
tar -xzf "$BACKUP_FILE" -C "$TMPDIR"
ls -la "$TMPDIR"
rm -rf "$TMPDIR"
```

---

## 3. Автоматизація резервного копіювання

### Linux — cron

```bash
# Відкрити редактор cron для поточного користувача
crontab -e

# Додати рядки:
# Щоденна інкрементальна копія о 03:00
0 3 * * 1-6 /usr/local/bin/gesture-mouse-backup.sh --incremental >> /var/log/gesture-mouse-backup.log 2>&1

# Щотижнева повна копія в неділю о 03:00
0 3 * * 0   /usr/local/bin/gesture-mouse-backup.sh --full >> /var/log/gesture-mouse-backup.log 2>&1
```

Встановити скрипт:
```bash
sudo cp docs/scripts/backup.sh /usr/local/bin/gesture-mouse-backup.sh
sudo chmod +x /usr/local/bin/gesture-mouse-backup.sh
```

### Windows — Task Scheduler

```powershell
# Створити задачу для щоденного резервного копіювання
$action = New-ScheduledTaskAction `
    -Execute "powershell.exe" `
    -Argument "-NonInteractive -File C:\GestureMouse\scripts\backup.ps1"

$trigger = New-ScheduledTaskTrigger -Daily -At "03:00"

$settings = New-ScheduledTaskSettingsSet `
    -RunOnlyIfNetworkAvailable:$false `
    -StartWhenAvailable

Register-ScheduledTask `
    -TaskName "GestureMouseBackup" `
    -Action $action `
    -Trigger $trigger `
    -Settings $settings `
    -RunLevel Highest

# Перевірити
Get-ScheduledTask -TaskName "GestureMouseBackup"
```

---

## 4. Процедура відновлення

### 4.1 Повне відновлення системи

```bash
# 1. Зупинити застосунок
systemctl --user stop gesture-mouse 2>/dev/null || pkill gesture_mouse 2>/dev/null

# 2. Знайти останню валідну копію
LATEST_BACKUP=$(ls -t $HOME/.gesture-mouse-backups/backup-*.tar.gz 2>/dev/null | head -1)
echo "Restoring from: $LATEST_BACKUP"

# 3. Перевірити цілісність
tar -tzf "$LATEST_BACKUP" > /dev/null || { echo "ERROR: Backup corrupted!"; exit 1; }

# 4. Розпакувати
TMPDIR=$(mktemp -d)
tar -xzf "$LATEST_BACKUP" -C "$TMPDIR"
BACKUP_CONTENT=$(ls "$TMPDIR")

# 5. Відновити конфігурацію
mkdir -p ~/.config/gesture-mouse
cp -r "$TMPDIR/$BACKUP_CONTENT/user-config/." ~/.config/gesture-mouse/
echo "Config restored"

# 6. Відновити бінарник (якщо потрібно)
if [ -f "$TMPDIR/$BACKUP_CONTENT/gesture_mouse_binary" ]; then
    sudo cp "$TMPDIR/$BACKUP_CONTENT/gesture_mouse_binary" /usr/local/bin/gesture_mouse
    sudo chmod +x /usr/local/bin/gesture_mouse
    echo "Binary restored"
fi

# 7. Очистити та запустити
rm -rf "$TMPDIR"
systemctl --user start gesture-mouse
gesture_mouse --version
echo "Recovery complete"
```

### 4.2 Вибіркове відновлення (тільки конфіг)

```bash
# Відновити лише файл конфігурації
BACKUP_FILE="$HOME/.gesture-mouse-backups/backup-TIMESTAMP.tar.gz"
TMPDIR=$(mktemp -d)
tar -xzf "$BACKUP_FILE" -C "$TMPDIR"

# Знайти файл конфігу всередині архіву
CONFIG_FILE=$(find "$TMPDIR" -name "settings.txt" | head -1)
echo "Found config: $CONFIG_FILE"

# Показати вміст для перевірки
cat "$CONFIG_FILE"

# Підтвердити та відновити
read -p "Restore this config? [y/N] " confirm
if [[ "$confirm" == "y" ]]; then
    cp "$CONFIG_FILE" ~/.config/gesture-mouse/settings.txt
    echo "Config restored"
fi

rm -rf "$TMPDIR"
```

### 4.3 Тестування відновлення

```bash
# Раз на місяць виконувати тестове відновлення
# у ізольованому середовищі (окрема VM або container)

BACKUP_FILE=$(ls -t $HOME/.gesture-mouse-backups/backup-*.tar.gz | head -1)
TEST_DIR=$(mktemp -d)

echo "=== Test restore of: $BACKUP_FILE ==="

# Розпакувати
tar -xzf "$BACKUP_FILE" -C "$TEST_DIR"

# Перевірити наявність всіх файлів
BACKUP_CONTENT=$(ls "$TEST_DIR")
echo "Contents:"
ls -la "$TEST_DIR/$BACKUP_CONTENT/"

# Перевірити бінарник (якщо є)
if [ -f "$TEST_DIR/$BACKUP_CONTENT/gesture_mouse_binary" ]; then
    file "$TEST_DIR/$BACKUP_CONTENT/gesture_mouse_binary"
    echo "Binary: OK"
fi

# Перевірити конфіг
if [ -f "$TEST_DIR/$BACKUP_CONTENT/user-config/settings.txt" ]; then
    echo "Config file:"
    cat "$TEST_DIR/$BACKUP_CONTENT/user-config/settings.txt"
    echo "Config: OK"
fi

rm -rf "$TEST_DIR"
echo "=== Test restore PASSED ==="
```
