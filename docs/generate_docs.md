# Генерація документації — GestureMouse

## 1. Огляд інструментів

| Інструмент | Мова | Формати виводу | Підтримка діаграм |
|---|---|---|---|
| **Doxygen** ✅ (обрано) | C/C++ | HTML, PDF, XML | DOT/Graphviz |
| Sphinx | Python/C++ | HTML, PDF | PlantUML |
| Breathe | C++ (через Sphinx) | HTML, PDF | — |
| Cldoc | C/C++ | HTML, JSON | — |

**Чому Doxygen:**
- Нативна підтримка C++ (розуміє шаблони, неймспейси, препроцесор)
- Генерує граф залежностей класів автоматично
- Стандарт де-факто для C++-проєктів
- Підтримує Markdown у коментарях
- Вбудований пошук у HTML-виводі

---

## 2. Встановлення

### Windows

```powershell
# Варіант 1 — winget (рекомендовано)
winget install doxygen.doxygen
winget install Graphviz.Graphviz   # для діаграм класів

# Варіант 2 — офіційний інсталятор
# https://www.doxygen.nl/download.html
# https://graphviz.org/download/

# Перевірка
doxygen --version
dot -V
```

### Ubuntu / WSL

```bash
sudo apt update
sudo apt install doxygen graphviz

# Перевірка
doxygen --version
dot -V
```

---

## 3. Швидкий старт

```bash
# З кореня репозиторію:
doxygen Doxyfile

# Відкрити результат:
# Windows:
start docs/generated/html/index.html

# Linux:
xdg-open docs/generated/html/index.html
```

---

## 4. Через скрипти проєкту

```bash
# Linux / WSL
./scripts/generate_docs.sh              # генерація
./scripts/generate_docs.sh --open       # генерація + відкрити браузер
./scripts/generate_docs.sh --check      # тільки перевірити warnings
```

```powershell
# Windows PowerShell
.\scripts\generate_docs.ps1             # генерація
.\scripts\generate_docs.ps1 -Open       # генерація + відкрити браузер
```

---

## 5. Через CMake

```bash
# Потрібно: cmake -B build (вже зроблено)
cmake --build build --target docs        # генерація
cmake --build build --target docs-check  # перевірка warnings
```

---

## 6. Структура виводу

```
docs/generated/
├── html/
│   ├── index.html          ← головна сторінка (README.md)
│   ├── annotated.html      ← список всіх класів
│   ├── files.html          ← список всіх файлів
│   ├── classDiagram.svg    ← діаграми класів (Graphviz)
│   └── search/             ← пошуковий індекс
├── xml/                    ← XML (для інтеграції зі Sphinx/Breathe)
└── doxygen-warnings.log    ← попередження (0 = ідеал)
```

---

## 7. Стандарти документування Doxygen для C++

### 7.1 Коментар файлу (кожен .h і .cpp)

```cpp
/**
 * @file ClassName.h
 * @brief Одна коротка фраза що описує вміст файлу.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * Детальний опис: архітектурні рішення, алгоритми, обмеження.
 */
```

### 7.2 Коментар класу

```cpp
/**
 * @brief Коротка фраза (відображається в списку класів).
 *
 * @details
 * Детальний опис з прикладом:
 * @code{.cpp}
 * MyClass obj;
 * obj.doSomething();
 * @endcode
 */
class MyClass { ... };
```

### 7.3 Коментар методу

```cpp
/**
 * @brief Що робить метод (одне речення).
 *
 * @details Детальний алгоритм, якщо необхідно.
 *
 * @param paramName  Опис параметра.
 * @param[out] result Вихідний параметр.
 * @param[in,out] buf Вхідно-вихідний параметр.
 *
 * @return Що повертає метод.
 *
 * @throws std::runtime_error якщо X.
 * @pre  Передумова: об'єкт ініціалізований.
 * @post Постумова: стан змінено.
 *
 * @note Важлива нотатка для користувача.
 * @warning Попередження про небезпечне використання.
 * @todo Що планується реалізувати.
 * @deprecated Замінений на newMethod(). Буде видалений у v2.0.
 *
 * @see RelatedClass::relatedMethod()
 */
```

### 7.4 Коментар члена класу

```cpp
float m_smoothing{0.3F}; ///< Коефіцієнт EMA-згладжування [0..1]
```

### 7.5 Групування

```cpp
/// @name Публічний інтерфейс
/// @{
void start();
void stop();
/// @}
```

### 7.6 Посилання та перехресні посилання

```cpp
// Посилання на клас:
/// @see GestureRecognizer
// Посилання на метод:
/// @see GestureRecognizer::process()
// Посилання на файл:
/// @see GestureRecognizer.h
```

---

## 8. Перевірка якості документації

### 8.1 Doxygen warnings (вбудований)

```bash
doxygen Doxyfile 2>&1 | grep -i warning | wc -l
```
Мета: **0 warnings** у `docs/generated/doxygen-warnings.log`.

### 8.2 Скрипт перевірки покриття

```bash
./scripts/check_doc_coverage.sh
```

Виводить:
```
Classes documented:    12/12 (100%)
Methods documented:    48/50 (96%)
Files documented:      10/10 (100%)
Undocumented methods:
  - HandTracker::someMethod()
  - GestureClassifier::helper()
```

---

## 9. Правила для команди (внесок у проєкт)

> **Правило 1:** Будь-який новий публічний метод або клас **обов'язково** має Doxygen-коментар з `@brief`, `@param`, `@return`.

> **Правило 2:** Якщо ти змінюєш поведінку методу — оновлюй коментар у **тому ж коміті**.

> **Правило 3:** Запускай `doxygen Doxyfile` перед `git push`. Якщо з'явились нові warnings — виправ їх.

> **Правило 4:** Складні алгоритми документуй у `@details` з поясненням **чому**, а не лише **що**.

---

## 10. CI/CD — автоматична публікація (GitHub Pages)

Документація автоматично генерується та публікується на GitHub Pages при кожному push в `main`.

**Адреса:** `https://cillianmorphine.github.io/gesture-mouse/docs/`

Налаштування: `.github/workflows/docs.yml`

```yaml
# Тригер: push у main
# Кроки: install doxygen → doxygen Doxyfile → deploy to gh-pages
```

Деталі CI/CD — у файлі `.github/workflows/docs.yml`.
