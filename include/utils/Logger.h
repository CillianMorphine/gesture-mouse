#pragma once
/**
 * @file Logger.h
 * @brief Легковажний потокобезпечний логер.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * ### Використання
 * @code{.cpp}
 * // Ініціалізація (один раз у main)
 * Logger::init("gesture_mouse.log");
 *
 * // Логування через макроси
 * LOG_INFO("Application started");
 * LOG_WARN("Camera index " + std::to_string(idx) + " not found, using 0");
 * LOG_ERROR("Failed to open config: " + path);
 * @endcode
 *
 * ### Формат виводу
 * ```
 * [2025-01-15 14:23:01] [INFO ] Application started (src/main.cpp:12)
 * [2025-01-15 14:23:01] [WARN ] Camera not found   (src/core/App.cpp:45)
 * ```
 *
 * ### Потокобезпека
 * Метод log() захищений `std::mutex` — безпечний для виклику
 * з будь-якого потоку одночасно.
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

/**
 * @brief Рівень важливості повідомлення логера.
 * @details Значення впорядковані від менш до більш критичних.
 *          У майбутньому можна відфільтрувати повідомлення нижче порога.
 */
enum class LogLevel : uint8_t {
    DEBUG = 0, ///< Детальна debug-інформація (тільки для розробки)
    INFO  = 1, ///< Звичайні інформаційні повідомлення
    WARN  = 2, ///< Попередження про нештатні, але не критичні ситуації
    ERROR = 3, ///< Критичні помилки (зазвичай передують аварійному завершенню)
};

/**
 * @brief Статичний потокобезпечний логер з виводом у файл і stdout.
 *
 * @details
 * Всі методи статичні — логер є **singleton за поведінкою**,
 * але реалізований без патерну Singleton (простіше тестувати).
 *
 * @note Клас не підлягає інстанціюванню (конструктор видалений).
 */
class Logger {
public:
    Logger() = delete; ///< Забороняємо створення об'єктів
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Ініціалізує логер: відкриває файл для запису.
     *
     * @param filePath Шлях до лог-файлу. Якщо файл існує — дописує (append).
     *
     * @note Якщо файл не вдалося відкрити — логер продовжує виводити у stdout.
     *       Виклик init() двічі закриває попередній файл і відкриває новий.
     *
     * @par Приклад:
     * @code{.cpp}
     * Logger::init("logs/gesture_mouse.log");
     * @endcode
     */
    static void init(const std::string& filePath);

    /**
     * @brief Записує повідомлення з рівнем, часовою міткою та позицією в коді.
     *
     * @details
     * Зазвичай викликається через макроси LOG_*, які автоматично
     * підставляють `__FILE__` та `__LINE__`.
     *
     * Формат рядка: `[TIMESTAMP] [LEVEL] message (file:line)`
     *
     * @param level   Рівень важливості (DEBUG / INFO / WARN / ERROR).
     * @param message Текст повідомлення (UTF-8).
     * @param file    Ім'я файлу (зазвичай __FILE__).
     * @param line    Номер рядка (зазвичай __LINE__).
     *
     * @thread_safety Потокобезпечний (захищений std::mutex).
     */
    static void log(LogLevel level, const std::string& message,
                    const char* file, int line);

    /**
     * @brief Перевіряє чи логер ініціалізований (файл відкрито).
     * @return true якщо лог-файл успішно відкрито.
     */
    [[nodiscard]] static bool isInitialized() noexcept { return s_file.is_open(); }

private:
    [[nodiscard]] static std::string levelToString(LogLevel l);
    [[nodiscard]] static std::string timestamp();

    static std::ofstream s_file;
    static std::mutex    s_mutex;
};

/**
 * @defgroup LogMacros Макроси логування
 * @brief Зручні макроси для виклику Logger::log() з автоматичним __FILE__/__LINE__.
 *
 * @details
 * Обгортка `do { ... } while(false)` дозволяє безпечно використовувати
 * макроси в конструкціях `if/else` без фігурних дужок:
 * @code{.cpp}
 * if (error)
 *     LOG_ERROR("Something went wrong");  // безпечно
 * else
 *     LOG_INFO("All good");               // безпечно
 * @endcode
 * @{
 */
/** @brief Логування debug-повідомлення. */
#define LOG_DEBUG(msg) do { Logger::log(LogLevel::DEBUG, (msg), __FILE__, __LINE__); } while(false)
/** @brief Логування інформаційного повідомлення. */
#define LOG_INFO(msg)  do { Logger::log(LogLevel::INFO,  (msg), __FILE__, __LINE__); } while(false)
/** @brief Логування попередження. */
#define LOG_WARN(msg)  do { Logger::log(LogLevel::WARN,  (msg), __FILE__, __LINE__); } while(false)
/** @brief Логування критичної помилки. */
#define LOG_ERROR(msg) do { Logger::log(LogLevel::ERROR, (msg), __FILE__, __LINE__); } while(false)
/** @} */ // end of LogMacros
