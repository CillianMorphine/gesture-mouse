# Розгортання у виробничому середовищі (Production Deployment)

> **Для кого цей документ:** release engineer, DevOps-спеціаліст, системний адміністратор.
> Цей документ описує повний процес розгортання GestureMouse на машину кінцевого користувача
> або корпоративний парк ПК.

---

## Особливості проєкту

GestureMouse є **десктопною системною утилітою**, а не серверним застосунком.
Це означає:
- Розгортання відбувається **на машині кінцевого користувача**, а не на сервері
- Немає необхідності у налаштуванні веб-сервера, СУБД або мережевих портів
- "Production environment" = машина користувача з встановленою ОС та камерою

---

## 1. Вимоги до апаратного забезпечення

### Мінімальні вимоги

| Компонент | Мінімум | Рекомендовано |
|---|---|---|
| **CPU** | Intel Core i3 або AMD Ryzen 3 (x64, 2 ядра) | Intel Core i5 / AMD Ryzen 5 (4+ ядра) |
| **RAM** | 2 ГБ вільної RAM | 4 ГБ і більше |
| **Диск** | 500 МБ вільного місця | 1 ГБ (враховуючи логи) |
| **Відеокамера** | USB 2.0, 640×480, 15 fps | USB 3.0, 1280×720, 30 fps |
| **Архітектура** | x86-64 (AMD64) | x86-64 |
| **GPU** | Не потрібна | Не потрібна |

### Перевірка сумісності камери

```bash
# Linux
v4l2-ctl --list-devices
v4l2-ctl --device /dev/video0 --list-formats-ext

# Windows PowerShell
Get-PnpDevice -Class Camera | Select-Object FriendlyName, Status
```

---

## 2. Необхідне програмне забезпечення

### Windows 10 / 11

| ПЗ | Версія | Обов'язково | Примітка |
|---|---|---|---|
| Windows 10/11 | 1903+ (x64) | Так | |
| Visual C++ Redistributable 2019 | 14.x | Так | Якщо не зібрано статично |
| OpenCV Runtime DLLs | 4.x | Так | `opencv_world4xx.dll` |
| DirectShow drivers | — | Так | Зазвичай вже є у Windows |

### Ubuntu 20.04 / 22.04

| Пакет | Версія | Команда встановлення |
|---|---|---|
| libopencv-core4.x | 4.x | `sudo apt install libopencv-core4.2` |
| libopencv-videoio4.x | 4.x | `sudo apt install libopencv-videoio4.2` |
| libopencv-imgproc4.x | 4.x | `sudo apt install libopencv-imgproc4.2` |
| libxtst6 | будь-яка | `sudo apt install libxtst6` |
| libx11-6 | будь-яка | `sudo apt install libx11-6` |
| v4l-utils | будь-яка | `sudo apt install v4l-utils` |

---

## 3. Налаштування мережі

GestureMouse **не використовує мережеві з'єднання** у штатному режимі.
Жодних вхідних або вихідних портів відкривати не потрібно.

Якщо корпоративний firewall блокує оновлення — для завантаження нових версій
потрібен HTTP/HTTPS (порт 443) до `github.com`.

---

## 4. Розгортання коду

### Варіант A: Розгортання скомпільованого бінарника (рекомендовано)

Використовуйте для масового розгортання на парку машин.

```bash
# 1. Отримати архів релізу з GitHub Releases
#    https://github.com/CillianMorphine/gesture-mouse/releases/latest

# Linux: розпакувати і встановити
tar -xzf gesture-mouse-v1.0.0-linux-x64.tar.gz
sudo cp gesture_mouse /usr/local/bin/
sudo cp assets/gesture-mouse.desktop /usr/share/applications/
sudo mkdir -p /etc/gesture-mouse
sudo cp assets/default_config.txt /etc/gesture-mouse/settings.txt
```

```powershell
# Windows: розпакувати і встановити
Expand-Archive gesture-mouse-v1.0.0-win-x64.zip -DestinationPath "C:\Program Files\GestureMouse"
# Додати до PATH або створити ярлик на робочому столі
```

### Варіант B: Збірка з вихідного коду

```bash
# 1. Клонувати репозиторій
git clone https://github.com/CillianMorphine/gesture-mouse.git
cd gesture-mouse

# 2. Переключитись на тег релізу
git checkout v1.0.0

# 3. Зібрати у режимі Release
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build -j$(nproc)

# 4. Встановити (Linux)
sudo cmake --install build --prefix /usr/local
```

### Розгортання конфігурації

```bash
# Системна конфігурація (для всіх користувачів)
sudo mkdir -p /etc/gesture-mouse
sudo cp assets/default_config.txt /etc/gesture-mouse/settings.txt

# Або конфігурація для конкретного користувача
mkdir -p ~/.config/gesture-mouse
cp assets/default_config.txt ~/.config/gesture-mouse/settings.txt
```

Порядок пошуку конфігурації:
1. `./config/settings.txt` (локальна, поряд з .exe)
2. `~/.config/gesture-mouse/settings.txt` (користувацька)
3. `/etc/gesture-mouse/settings.txt` (системна)
4. Вбудовані значення за замовчуванням

---

## 5. Налаштування автозапуску

### Linux — systemd user service

```bash
# Створити файл сервісу
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/gesture-mouse.service << 'EOF'
[Unit]
Description=GestureMouse — gesture-based cursor control
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/local/bin/gesture_mouse
Restart=on-failure
RestartSec=5
Environment=DISPLAY=:0

[Install]
WantedBy=default.target
EOF

# Увімкнути та запустити
systemctl --user enable gesture-mouse
systemctl --user start gesture-mouse
systemctl --user status gesture-mouse
```

### Windows — автозапуск через реєстр

```powershell
# Додати до автозапуску поточного користувача
$regPath = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Set-ItemProperty -Path $regPath -Name "GestureMouse" `
    -Value "C:\Program Files\GestureMouse\gesture_mouse.exe"
```

---

## 6. Перевірка працездатності

Виконати після кожного розгортання.

```bash
# 1. Перевірити що бінарник запускається
gesture_mouse --version
# Очікується: GestureMouse v1.0.0

# 2. Перевірити доступ до камери
gesture_mouse --check-camera
# Очікується: Camera 0: OK (640x480 @ 30fps)

# 3. Перевірити запис логів
gesture_mouse --debug &
sleep 5
kill %1
cat gesture_mouse.log | head -20
# Очікується: [INFO] GestureMouse starting...
#             [INFO] Camera opened successfully
#             [INFO] Entering main loop

# 4. Перевірити права доступу (Linux)
ls -la /dev/video0
groups $USER | grep video
# Очікується: поточний користувач у групі video

# 5. Smoke test — запустити і дочекатися 3 секунди без crash
timeout 3 gesture_mouse --no-tray && echo "PASS" || echo "FAIL"
```

### Чек-лист перевірки після розгортання

- [ ] `gesture_mouse --version` виводить правильну версію
- [ ] `gesture_mouse --check-camera` знаходить камеру
- [ ] Лог-файл `gesture_mouse.log` створюється і містить `[INFO]`
- [ ] Іконка в системному треї з'являється
- [ ] Курсор реагує на рух руки перед камерою
- [ ] Автозапуск (якщо налаштовано) спрацьовує після перезавантаження
