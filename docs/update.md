# Процедура оновлення системи

> **Для кого цей документ:** release engineer, DevOps-спеціаліст.
> Описує покрокову процедуру оновлення GestureMouse від однієї версії до іншої.

---

## Перед початком: перевірка поточної версії

```bash
gesture_mouse --version
# Наприклад: GestureMouse v1.0.0 (build: 2025-01-15)

# Перевірити яка версія буде встановлена
cat CHANGELOG.md | head -20
```

---

## 1. Підготовка до оновлення

### 1.1 Резервне копіювання

**Завжди** виконувати перед оновленням. Детально — у [`backup.md`](backup.md).

```bash
# Швидке резервне копіювання перед оновленням
BACKUP_DIR="$HOME/.gesture-mouse-backups/pre-update-$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Зберегти конфіг
cp -r ~/.config/gesture-mouse "$BACKUP_DIR/config" 2>/dev/null || true
cp -r /etc/gesture-mouse "$BACKUP_DIR/system-config" 2>/dev/null || true

# Зберегти поточний бінарник
cp "$(which gesture_mouse)" "$BACKUP_DIR/gesture_mouse_$(gesture_mouse --version 2>/dev/null | grep -oP 'v[\d.]+')" 2>/dev/null || true

# Зберегти логи
cp gesture_mouse.log "$BACKUP_DIR/" 2>/dev/null || true

echo "Backup created: $BACKUP_DIR"
```

### 1.2 Перевірка сумісності

```bash
# Перевірити CHANGELOG нової версії на breaking changes
curl -s https://raw.githubusercontent.com/CillianMorphine/gesture-mouse/main/CHANGELOG.md \
  | head -50

# Перевірити версію OpenCV (потрібна >= 4.0)
pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv

# Перевірити версію ОС
lsb_release -a    # Linux
winver            # Windows
```

### 1.3 Час простою

GestureMouse — десктопна утиліта. "Час простою" = час між зупинкою
поточної версії та запуском нової. Зазвичай < 30 секунд.

**Рекомендований час:** виконувати оновлення коли користувач не активно
використовує систему (перерва, обідня пауза).

---

## 2. Процес оновлення

### 2.1 Зупинка поточної версії

```bash
# Linux: зупинити systemd-сервіс
systemctl --user stop gesture-mouse
systemctl --user status gesture-mouse
# Очікується: Active: inactive (dead)

# Linux: якщо запущено як процес (без systemd)
pkill -SIGTERM gesture_mouse
sleep 2
# Примусова зупинка якщо не зупинився
pkill -SIGKILL gesture_mouse 2>/dev/null || true

# Windows PowerShell
Stop-Process -Name "gesture_mouse" -Force -ErrorAction SilentlyContinue
```

### 2.2 Розгортання нового коду

#### Варіант A: Оновлення через Git (якщо розгорнуто з вихідного коду)

```bash
cd /path/to/gesture-mouse

# Зберегти поточний коміт для можливого відкату
git rev-parse HEAD > /tmp/gesture_mouse_prev_commit.txt

# Отримати нову версію
git fetch origin
git checkout v1.1.0    # або конкретний тег

# Перевірити що отримали правильну версію
git log --oneline -1

# Перезібрати
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j$(nproc)

# Встановити
sudo cmake --install build --prefix /usr/local
```

#### Варіант B: Оновлення через бінарний архів

```bash
# Завантажити нову версію
wget https://github.com/CillianMorphine/gesture-mouse/releases/download/v1.1.0/gesture-mouse-v1.1.0-linux-x64.tar.gz

# Розпакувати
tar -xzf gesture-mouse-v1.1.0-linux-x64.tar.gz -C /tmp/gesture-mouse-new

# Замінити бінарник
sudo cp /tmp/gesture-mouse-new/gesture_mouse /usr/local/bin/gesture_mouse
sudo chmod +x /usr/local/bin/gesture_mouse

# Верифікація
gesture_mouse --version
# Очікується нова версія
```

### 2.3 Оновлення конфігурації

```bash
# Порівняти нову конфігурацію за замовчуванням з поточною
diff assets/default_config.txt ~/.config/gesture-mouse/settings.txt

# Якщо є нові параметри — додати їх зі значеннями за замовчуванням
# НЕ замінювати увесь файл — зберегти налаштування користувача

# Приклад: додати новий параметр gesture.timeout якщо його немає
if ! grep -q "gesture.timeout" ~/.config/gesture-mouse/settings.txt; then
    echo "gesture.timeout=500" >> ~/.config/gesture-mouse/settings.txt
    echo "Added new config parameter: gesture.timeout=500"
fi
```

### 2.4 Перевірка сумісності конфігурації

```bash
# Запустити у режимі валідації конфігу (без запуску головного циклу)
gesture_mouse --validate-config ~/.config/gesture-mouse/settings.txt
# Очікується: Config OK — all parameters valid
```

---

## 3. Перевірка після оновлення

```bash
# 1. Версія
gesture_mouse --version
# Очікується: нова версія (наприклад, v1.1.0)

# 2. Камера
gesture_mouse --check-camera
# Очікується: Camera 0: OK

# 3. Smoke test
timeout 5 gesture_mouse --no-tray && echo "PASS" || echo "FAIL"

# 4. Перевірити логи на помилки
gesture_mouse --debug &
sleep 3
kill %1
grep -i "error\|critical\|fatal" gesture_mouse.log
# Очікується: порожній вивід (немає помилок)
```

### Запуск після перевірки

```bash
# Linux: запустити через systemd
systemctl --user start gesture-mouse
systemctl --user status gesture-mouse

# Windows
Start-Process "C:\Program Files\GestureMouse\gesture_mouse.exe"
```

---

## 4. Процедура відкату (Rollback)

Виконувати якщо після оновлення система не працює коректно.

### Сценарій A: Відкат через Git

```bash
# Зупинити нову версію
systemctl --user stop gesture-mouse || pkill gesture_mouse

# Повернутись до попереднього коміту
cd /path/to/gesture-mouse
PREV_COMMIT=$(cat /tmp/gesture_mouse_prev_commit.txt)
git checkout "$PREV_COMMIT"

# Перезібрати попередню версію
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local

echo "Rollback complete. Version: $(gesture_mouse --version)"
```

### Сценарій B: Відкат через збережений бінарник

```bash
# Зупинити поточну версію
systemctl --user stop gesture-mouse

# Знайти збережений бінарник
BACKUP_BIN=$(ls -t $HOME/.gesture-mouse-backups/pre-update-*/gesture_mouse_* 2>/dev/null | head -1)

if [ -n "$BACKUP_BIN" ]; then
    sudo cp "$BACKUP_BIN" /usr/local/bin/gesture_mouse
    sudo chmod +x /usr/local/bin/gesture_mouse
    echo "Rollback complete. Version: $(gesture_mouse --version)"
else
    echo "ERROR: No backup binary found!"
fi

# Відновити конфіг якщо потрібно
BACKUP_CFG=$(ls -td $HOME/.gesture-mouse-backups/pre-update-*/config 2>/dev/null | head -1)
if [ -n "$BACKUP_CFG" ]; then
    cp -r "$BACKUP_CFG" ~/.config/gesture-mouse
fi

# Запустити
systemctl --user start gesture-mouse
```

### Сценарій C: Відкат на Windows

```powershell
# Зупинити
Stop-Process -Name "gesture_mouse" -Force -ErrorAction SilentlyContinue

# Знайти backup (якщо виконувався backup.ps1)
$backupDir = Get-ChildItem "$env:USERPROFILE\.gesture-mouse-backups\pre-update-*" |
             Sort-Object LastWriteTime -Descending | Select-Object -First 1

if ($backupDir) {
    $backupBin = Get-ChildItem "$($backupDir.FullName)\gesture_mouse*.exe" |
                 Select-Object -First 1
    if ($backupBin) {
        Copy-Item $backupBin.FullName "C:\Program Files\GestureMouse\gesture_mouse.exe" -Force
        Write-Host "Rollback complete"
    }
}

# Запустити
Start-Process "C:\Program Files\GestureMouse\gesture_mouse.exe"
```

### Чек-лист відкату

- [ ] Нова версія зупинена
- [ ] Попередній бінарник відновлено
- [ ] Конфігурація відновлена (якщо змінювалась)
- [ ] Попередня версія успішно запускається
- [ ] Smoke test пройдений
- [ ] Заведено тікет з описом проблеми (для виправлення в наступному релізі)
