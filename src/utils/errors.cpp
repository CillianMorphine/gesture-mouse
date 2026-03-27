/**
 * @file errors.cpp
 * @brief Реалізація системи обробки помилок і локалізації GestureMouse.
 */

#include "errors.hpp"
#include <unordered_map>

namespace gm {

// ─── Активна мова ────────────────────────────────────────────────────────────

Locale& activeLocale() {
    static Locale loc = Locale::UK; // за замовчуванням — українська
    return loc;
}

// ─── Таблиці локалізованих повідомлень ───────────────────────────────────────

namespace {

struct Messages {
    std::string uk; ///< Українська
    std::string en; ///< Англійська
};

/**
 * @brief Словник повідомлень для кінцевих користувачів.
 *
 * Повідомлення НАВМИСНО не містять технічних деталей (кодів, назв функцій).
 * Кожне повідомлення включає:
 *  1. Опис проблеми зрозумілою мовою.
 *  2. Рекомендовані дії.
 *  3. Пропозицію звернутися за допомогою.
 */
const std::unordered_map<ErrorCode, Messages> kUserMessages = {

    // ── Загальні ────────────────────────────────────────────────
    { ErrorCode::Unknown, {
        "Виникла непередбачена помилка.\n"
        "Спробуйте перезапустити програму. Якщо помилка повторюється — "
        "зверніться до розробника та повідомте ідентифікатор сесії.",
        "An unexpected error occurred.\n"
        "Try restarting the application. If the problem persists, "
        "please contact support and provide your session ID."
    }},
    { ErrorCode::InvalidArgument, {
        "Програма запущена з некоректними параметрами.\n"
        "Перевірте аргументи командного рядка (запустіть із --help).",
        "The application was started with invalid parameters.\n"
        "Check the command-line arguments (run with --help)."
    }},
    { ErrorCode::ConfigParseFail, {
        "Не вдалося прочитати файл налаштувань gesture_mouse.conf.\n"
        "Переконайтесь, що файл існує та не пошкоджений. "
        "Буде використано налаштування за замовчуванням.",
        "Could not read the configuration file gesture_mouse.conf.\n"
        "Ensure the file exists and is not corrupted. "
        "Default settings will be used."
    }},

    // ── Камера ──────────────────────────────────────────────────
    { ErrorCode::CameraNotFound, {
        "Веб-камеру не знайдено.\n"
        "Переконайтесь, що:\n"
        "  • камера підключена та розпізнана системою;\n"
        "  • до камери немає доступу в іншій програмі;\n"
        "  • встановлені драйвери камери.",
        "No webcam was found.\n"
        "Please ensure that:\n"
        "  • the camera is connected and recognized by the system;\n"
        "  • no other application is using the camera;\n"
        "  • camera drivers are installed."
    }},
    { ErrorCode::CameraReadFail, {
        "Помилка зчитування відеопотоку з камери.\n"
        "Перевірте підключення камери та спробуйте знову.",
        "Failed to read video from the camera.\n"
        "Check the camera connection and try again."
    }},
    { ErrorCode::CameraResolution, {
        "Камера не підтримує потрібну роздільну здатність.\n"
        "Для коректної роботи потрібна камера з роздільною здатністю "
        "не менше 640×480 та частотою 30 кадрів/с.",
        "The camera does not support the required resolution.\n"
        "A camera with at least 640×480 resolution at 30 fps is required."
    }},

    // ── Жести ───────────────────────────────────────────────────
    { ErrorCode::GestureInitFail, {
        "Не вдалося ініціалізувати систему розпізнавання жестів.\n"
        "Переконайтесь, що всі файли програми присутні та не пошкоджені.",
        "Failed to initialize the gesture recognition system.\n"
        "Ensure all application files are present and not corrupted."
    }},
    { ErrorCode::GestureTimeout, {
        "Система не змогла розпізнати жест вчасно.\n"
        "Переконайтесь, що рука добре освітлена та повністю у полі камери.",
        "Gesture recognition timed out.\n"
        "Ensure your hand is well-lit and fully within the camera view."
    }},
    { ErrorCode::GestureAmbiguous, {
        "Жест розпізнано з недостатньою впевненістю.\n"
        "Спробуйте виконати жест чіткіше або покращіть освітлення.",
        "Gesture was recognized with insufficient confidence.\n"
        "Try making the gesture more clearly or improve lighting."
    }},

    // ── Введення ────────────────────────────────────────────────
    { ErrorCode::InputInitFail, {
        "Не вдалося ініціалізувати модуль керування курсором.\n"
        "Спробуйте перезапустити програму від імені адміністратора.",
        "Failed to initialize the cursor control module.\n"
        "Try restarting the application as administrator."
    }},
    { ErrorCode::InputSendFail, {
        "Не вдалося виконати дію миші.\n"
        "Можливо, в системі активна захисна функція, "
        "що блокує симуляцію введення.",
        "Failed to perform the mouse action.\n"
        "A system security feature might be blocking input simulation."
    }},
    { ErrorCode::PermissionDenied, {
        "Недостатньо прав для керування курсором.\n"
        "Запустіть GestureMouse від імені адміністратора.",
        "Insufficient permissions to control the cursor.\n"
        "Run GestureMouse as administrator."
    }},

    // ── Системні ────────────────────────────────────────────────
    { ErrorCode::ResourceExhausted, {
        "Система вичерпала доступні ресурси.\n"
        "Закрийте непотрібні програми та спробуйте знову.",
        "The system has run out of resources.\n"
        "Close unnecessary applications and try again."
    }},
    { ErrorCode::ThreadStartFail, {
        "Не вдалося запустити фоновий процес.\n"
        "Перезапустіть програму. Якщо помилка повторюється, "
        "перевірте системні ресурси.",
        "Failed to start a background thread.\n"
        "Restart the application. If the issue persists, "
        "check system resources."
    }},
};

} // anonymous namespace

// ─── Реалізація getUserMessage ────────────────────────────────────────────────

std::string getUserMessage(ErrorCode code, Locale locale) {
    auto it = kUserMessages.find(code);
    if (it == kUserMessages.end()) {
        it = kUserMessages.find(ErrorCode::Unknown);
    }
    return locale == Locale::UK ? it->second.uk : it->second.en;
}

} // namespace gm
