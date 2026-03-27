#pragma once
/**
 * @file errors.hpp
 * @brief Система обробки помилок GestureMouse.
 *
 * Включає:
 *  - ієрархію власних виключень;
 *  - унікальні ідентифікатори помилок (ErrorCode);
 *  - контекстну інформацію (параметри, стан системи);
 *  - локалізовані повідомлення (Ukrainian / English);
 *  - зручні повідомлення для кінцевих користувачів (без технічних деталей);
 *  - макрос GM_THROW для автоматичного логування при кидку виключення.
 */

#include "logger.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace gm {

// ─── Коди помилок ────────────────────────────────────────────────────────────

/**
 * @brief Унікальні ідентифікатори помилок GestureMouse.
 *
 * Формат у логах: [GM-XXXX]
 * Дозволяє однозначно ідентифікувати тип помилки без читання тексту.
 */
enum class ErrorCode : uint32_t {
    // ── Загальні ────────────────────────────────────────────────
    Unknown           = 1000, ///< Невідома помилка (не слід використовувати явно)
    InvalidArgument   = 1001, ///< Некоректний аргумент функції або CLI
    ConfigParseFail   = 1002, ///< Помилка читання/парсингу конфігурації

    // ── Камера / відеопотік ──────────────────────────────────────
    CameraNotFound    = 2001, ///< Камеру не знайдено або не вдалося відкрити
    CameraReadFail    = 2002, ///< Збій читання кадру з камери
    CameraResolution  = 2003, ///< Непідтримувана роздільна здатність камери

    // ── Розпізнавання жестів ─────────────────────────────────────
    GestureInitFail   = 3001, ///< Помилка ініціалізації детектора жестів
    GestureTimeout    = 3002, ///< Таймаут розпізнавання жесту
    GestureAmbiguous  = 3003, ///< Жест не розпізнано однозначно (низька впевненість)

    // ── Симуляція введення (Win32 / X11) ─────────────────────────
    InputInitFail     = 4001, ///< Не вдалося ініціалізувати модуль введення
    InputSendFail     = 4002, ///< Помилка надсилання події миші/клавіатури
    PermissionDenied  = 4003, ///< Недостатньо прав для симуляції введення

    // ── Системні ─────────────────────────────────────────────────
    ResourceExhausted = 5001, ///< Вичерпано системні ресурси (пам'ять, потоки)
    ThreadStartFail   = 5002, ///< Не вдалося запустити фоновий потік
};

/**
 * @brief Перетворює ErrorCode у рядок формату GM-XXXX.
 */
inline std::string errorCodeStr(ErrorCode code) {
    return "GM-" + std::to_string(static_cast<uint32_t>(code));
}

// ─── Локалізація ─────────────────────────────────────────────────────────────

/** @brief Підтримувані мови повідомлень. */
enum class Locale { UK, EN };

/** @brief Активна мова (змінюється через setLocale()). */
Locale& activeLocale();

/** @brief Встановлює активну мову. */
inline void setLocale(Locale loc) { activeLocale() = loc; }

/**
 * @brief Повертає локалізоване повідомлення для кінцевого користувача.
 *
 * Повідомлення — зрозуміле, без технічних деталей.
 * @param code   Код помилки.
 * @param locale Мова (за замовчуванням — activeLocale()).
 */
std::string getUserMessage(ErrorCode code,
                           Locale locale = activeLocale());

// ─── Базовий клас виключень ───────────────────────────────────────────────────

/**
 * @brief Базове виключення GestureMouse.
 *
 * Несе:
 *  - унікальний код помилки (ErrorCode);
 *  - технічне повідомлення (для логів/розробників);
 *  - контекстну інформацію (параметри, стан);
 *  - ім'я модуля, де сталася помилка.
 */
class GestureMouseError : public std::runtime_error {
public:
    GestureMouseError(ErrorCode       code,
                      std::string     technical_msg,
                      std::string     context   = {},
                      std::string     module    = "core")
        : std::runtime_error(technical_msg)
        , code_(code)
        , technical_(std::move(technical_msg))
        , context_(std::move(context))
        , module_(std::move(module))
    {}

    ErrorCode        code()      const noexcept { return code_;      }
    std::string_view codeStr()   const noexcept { return code_str_;  }
    std::string_view technical() const noexcept { return technical_; }
    std::string_view context()   const noexcept { return context_;   }
    std::string_view module()    const noexcept { return module_;     }

    /** @brief Повідомлення для кінцевого користувача (локалізоване). */
    std::string userMessage(Locale loc = activeLocale()) const {
        return getUserMessage(code_, loc);
    }

    /** @brief Повний рядок для запису в лог. */
    std::string logMessage() const {
        std::string msg = "[" + errorCodeStr(code_) + "] [" + module_ + "] " + technical_;
        if (!context_.empty()) msg += " | Контекст: " + context_;
        return msg;
    }

private:
    ErrorCode   code_;
    std::string code_str_ = errorCodeStr(code_);
    std::string technical_;
    std::string context_;
    std::string module_;
};

// ─── Спеціалізовані виключення ───────────────────────────────────────────────

/** @brief Помилки, пов'язані з камерою. */
class CameraError : public GestureMouseError {
public:
    CameraError(ErrorCode code, std::string msg, std::string ctx = {})
        : GestureMouseError(code, std::move(msg), std::move(ctx), "camera") {}
};

/** @brief Помилки розпізнавання жестів. */
class GestureError : public GestureMouseError {
public:
    GestureError(ErrorCode code, std::string msg, std::string ctx = {})
        : GestureMouseError(code, std::move(msg), std::move(ctx), "gesture") {}
};

/** @brief Помилки симуляції введення. */
class InputError : public GestureMouseError {
public:
    InputError(ErrorCode code, std::string msg, std::string ctx = {})
        : GestureMouseError(code, std::move(msg), std::move(ctx), "input") {}
};

/** @brief Системні помилки (ресурси, потоки). */
class SystemError : public GestureMouseError {
public:
    SystemError(ErrorCode code, std::string msg, std::string ctx = {})
        : GestureMouseError(code, std::move(msg), std::move(ctx), "system") {}
};

// ─── Макрос безпечного кидку з логуванням ────────────────────────────────────

/**
 * @brief Логує помилку та кидає виключення.
 *
 * Використання:
 * @code
 *   GM_THROW(CameraError, ErrorCode::CameraNotFound,
 *            "Камеру з індексом 0 не знайдено",
 *            "camera_index=0, os=Windows");
 * @endcode
 */
#define GM_THROW(ExcType, code, msg, ctx)                                  \
    do {                                                                   \
        ExcType _ex(code, msg, ctx);                                       \
        GM_ERROR("{}", _ex.logMessage());                                  \
        throw _ex;                                                         \
    } while(0)

} // namespace gm
