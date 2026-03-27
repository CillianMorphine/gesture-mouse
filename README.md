# 🖱️ GestureMouse — Системна утиліта керування курсором жестами

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

Системна утиліта для безконтактного керування курсором миші за допомогою жестів рук,
розпізнаних через веб-камеру у режимі реального часу.

---

## Зміст

- [Архітектура системи](#архітектура-системи)
- [Швидкий старт](#швидкий-старт)
- [Детальна інструкція — Windows](#детальна-інструкція--windows)
- [Детальна інструкція — Linux Ubuntu](#детальна-інструкція--linux-ubuntu)
- [Збірка та запуск](#збірка-та-запуск)
- [Базові команди](#базові-команди)
- [Структура репозиторію](#структура-репозиторію)
- [Документація](#документація)

---

## Архітектура системи

GestureMouse є **десктопною системною утилітою** (не вебзастосунком).
Проєкт не використовує веб-сервер, СУБД, файлове сховище або сервіси кешування.

```
+----------------------------------------------------------------------+
|                        GestureMouse Process                          |
|                                                                      |
|  +---------------+   +----------------------+   +-----------------+  |
|  | ConfigManager |   |  GestureRecognizer   |   | MouseController |  |
|  | settings.txt  |   |  HandTracker         |   | Win32 SendInput |  |
|  | key=value     |   |  GestureClassifier   |   | Linux XTest     |  |
|  +-------+-------+   +-----------+----------+   +--------+--------+  |
|          |                       |                       |            |
|          +-------------------Application-----------------+            |
|                           (main loop ~60 fps)                        |
+------------------------------+---------------------------------------+
                               |
               +---------------+----------------+
               |        OS / Hardware           |
               |  WebCam (V4L2 / DirectShow)    |
               |  Display / Input Driver        |
               +--------------------------------+
```

**Компоненти системи:**

| Компонент | Опис |
|---|---|
| `Application` | Головний клас, координує модулі, реалізує головний цикл |
| `GestureRecognizer` | Фасад CV-підсистеми (HandTracker + GestureClassifier) |
| `HandTracker` | HSV-сегментація тілесного кольору, пошук контурів |
| `GestureClassifier` | Визначення типу жесту за ключовими точками |
| `MouseController` | Симуляція введення (Win32 SendInput / Linux XTest) |
| `ConfigManager` | Завантаження та збереження конфігурації (файл key=value) |
| `TrayIcon` | Іконка в системному треї з меню керування |
| `Logger` | Потокобезпечний логер (файл + stdout) |

**Зовнішні залежності:**

| Бібліотека | Версія | Призначення |
|---|---|---|
| OpenCV | 4.x | Захват відео, обробка зображень, HSV-сегментація |
| CMake | >= 3.15 | Крос-платформна система збірки |
| GTest | >= 1.11 | Модульні тести (опціонально) |

---

## Швидкий старт

> Якщо у вас свіже встановлена ОС — дивіться детальні інструкції нижче.

```bash
# Клонувати репозиторій
git clone https://github.com/CillianMorphine/gesture-mouse.git
cd gesture-mouse

# Linux: встановити залежності одною командою
sudo apt install -y cmake build-essential libopencv-dev libx11-dev libxtst-dev

# Зібрати проєкт
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
cmake --build build -j$(nproc)

# Запустити
./build/gesture_mouse
```

---

## Детальна інструкція — Windows

### Крок 1: Встановлення Git

1. Завантажити з https://git-scm.com/download/win
2. Встановити з параметрами за замовчуванням (залишити всі галочки)
3. Перевірити: відкрити PowerShell та виконати `git --version`

### Крок 2: Встановлення Visual Studio 2019

1. Завантажити **Visual Studio 2019 Community** з https://visualstudio.microsoft.com/vs/older-downloads/
2. При встановленні обрати workload: **"Desktop development with C++"**
3. Переконатися що встановлені: MSVC v142, Windows 10 SDK, CMake tools for Visual Studio

### Крок 3: Встановлення CMake (якщо не встановився з VS)

1. Завантажити з https://cmake.org/download/ — вибрати "Windows x64 Installer"
2. При встановленні обовʼязково обрати: **"Add CMake to the system PATH for all users"**
3. Перевірити: `cmake --version` (потрібна версія >= 3.15)

### Крок 4: Встановлення OpenCV 4

```powershell
# Варіант 1: через winget
winget install OpenCV.OpenCV

# Варіант 2: вручну
# 1. Завантажити https://github.com/opencv/opencv/releases
# 2. Запустити opencv-4.x.x-windows.exe
# 3. Розпакувати у C:\opencv
```

Налаштувати змінні середовища (Win+R -> sysdm.cpl -> Додатково -> Змінні середовища):
- Додати нову змінну: `OPENCV_DIR` = `C:\opencv\build`
- До змінної `Path` додати: `C:\opencv\build\x64\vc16\bin`

### Крок 5: Клонування репозиторію

```powershell
git clone https://github.com/CillianMorphine/gesture-mouse.git
cd gesture-mouse
```

### Крок 6: Збірка проєкту

Відкрити **Developer PowerShell for VS 2019** (або звичайний PowerShell):

```powershell
cmake -G "Visual Studio 16 2019" -A x64 `
      -DOpenCV_DIR="C:/opencv/build" `
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `
      -B build

cmake --build build --config Debug
```

### Крок 7: Запуск

```powershell
.\build\Debug\gesture_mouse.exe
```

> **Помилка про відсутній DLL?** Скопіюйте файл `opencv_world4xx.dll`
> з `C:\opencv\build\x64\vc16\bin\` до папки з `.exe`.

---

## Детальна інструкція — Linux Ubuntu

Перевірено на Ubuntu 20.04 LTS та 22.04 LTS.

### Крок 1: Оновлення системи та встановлення базових інструментів

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y git build-essential cmake pkg-config
```

### Крок 2: Встановлення OpenCV та системних залежностей

```bash
# OpenCV 4
sudo apt install -y \
    libopencv-dev \
    libopencv-core-dev \
    libopencv-highgui-dev \
    libopencv-imgproc-dev \
    libopencv-videoio-dev

# X11 для симуляції введення миші та клавіатури
sudo apt install -y libx11-dev libxtst-dev libxext-dev

# Google Test (для запуску тестів, опціонально)
sudo apt install -y libgtest-dev

# Doxygen (для генерації документації, опціонально)
sudo apt install -y doxygen graphviz

# Перевірити встановлення
cmake --version
pkg-config --modversion opencv4
```

### Крок 3: Клонування репозиторію

```bash
git clone https://github.com/CillianMorphine/gesture-mouse.git
cd gesture-mouse
```

### Крок 4: Налаштування конфігурації

```bash
# Створити папку config та скопіювати налаштування за замовчуванням
mkdir -p config
cp assets/default_config.txt config/settings.txt

# За бажанням відредагувати
nano config/settings.txt
```

Основні параметри конфігурації:

```ini
camera.index=0          # Індекс камери (0 = перша)
mouse.smoothing=0.3     # EMA-згладжування [0..1]
gesture.confidence=0.6  # Мінімальна впевненість жесту
debug.show_window=false # Debug-вікно OpenCV
```

### Крок 5: Збірка

```bash
# Через скрипт (найпростіший спосіб)
chmod +x scripts/build.sh
./scripts/build.sh Debug

# Або вручну
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -B build

cmake --build build -j$(nproc)
```

### Крок 6: Доступ до камери

```bash
# Перевірити що камера доступна
ls -la /dev/video*

# Якщо відмовлено у доступі — додати себе до групи video
sudo usermod -aG video $USER
# Потім вийти та знову зайти в систему
```

### Крок 7: Запуск

```bash
# Звичайний запуск
./build/gesture_mouse

# З debug-вікном (показує розпізнавання в реальному часі)
./build/gesture_mouse --debug

# Вказати індекс камери явно
./build/gesture_mouse --camera 1
```

---

## Збірка та запуск

### Режими збірки

```bash
# Debug — для розробки (символи відладки, повільніше)
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build

# Release — для виробництва (оптимізований, без символів)
cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build

# RelWithDebInfo — оптимізований з символами (для профілювання)
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build && cmake --build build
```

### Запуск тестів

```bash
# Зібрати і запустити тести
cmake --build build --target gesture_mouse_tests
cd build && ctest --output-on-failure -V
```

### Статичний аналіз (лінтер)

```bash
# Перевірка (потрібен clang-tidy)
./scripts/lint.sh

# З автоматичним виправленням
./scripts/lint.sh --fix
```

### Генерація документації

```bash
# Потрібен Doxygen
doxygen Doxyfile
# Відкрити: xdg-open docs/generated/html/index.html
```

---

## Базові команди

| Команда | Опис |
|---|---|
| `./scripts/build.sh` | Release збірка (Linux) |
| `./scripts/build.sh Debug` | Debug збірка (Linux) |
| `.\scripts\build.bat` | Release збірка (Windows) |
| `./scripts/lint.sh` | Статичний аналіз clang-tidy + cppcheck |
| `./scripts/lint.sh --fix` | Авто-виправлення проблем |
| `./scripts/generate_docs.sh` | Генерація Doxygen HTML |
| `ctest --test-dir build` | Запуск всіх тестів |
| `git log --oneline -10` | Останні 10 комітів |

---

## Структура репозиторію

```
gesture-mouse/
├── src/                    # Вихідний код C++
│   ├── main.cpp            # Точка входу
│   ├── core/               # Application, ConfigManager
│   ├── gesture/            # GestureRecognizer, HandTracker, GestureClassifier
│   ├── input/              # MouseController, InputSimulator
│   ├── ui/                 # TrayIcon, SettingsWindow
│   └── utils/              # Logger, Timer
├── include/                # Заголовкові файли
├── tests/                  # Google Test модульні тести
├── docs/                   # Документація DevOps та розробника
│   ├── api/                # Структура сайту онлайн документації
│   ├── deployment.md       # Розгортання у production
│   ├── update.md           # Процедура оновлення
│   ├── backup.md           # Резервне копіювання
│   ├── linting.md          # Статичний аналіз
│   ├── generate_docs.md    # Генерація документації
│   └── scripts/            # DevOps automation scripts
├── assets/                 # Іконки, конфігурація за замовчуванням
├── scripts/                # Скрипти збірки та розробки
├── .clang-tidy             # Конфіг clang-tidy
├── .clang-format           # Конфіг clang-format
├── .gitignore
├── CMakeLists.txt
├── Doxyfile
├── LICENSE
└── README.md
```

---

## Документація

| Файл | Зміст |
|---|---|
| [`docs/deployment.md`](docs/deployment.md) | Розгортання у production-середовищі |
| [`docs/update.md`](docs/update.md) | Процедура оновлення застосунку |
| [`docs/backup.md`](docs/backup.md) | Стратегія резервного копіювання |
| [`docs/linting.md`](docs/linting.md) | Налаштування та використання лінтерів |
| [`docs/generate_docs.md`](docs/generate_docs.md) | Генерація HTML-документації |

🔗 **[Онлайн документація](https://cillianmorphine.github.io/gesture-mouse/api/)**
