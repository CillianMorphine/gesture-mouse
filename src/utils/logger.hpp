#pragma once
/**
 * @file logger.hpp
 * @brief Система логування GestureMouse на базі spdlog.
 *
 * Надає:
 *  - глобальний логер із кольоровим виводом у консоль та ротаційним файловим sink-ом;
 *  - визначення рівня логування без перекомпіляції (env-змінна GESTURE_LOG_LEVEL
 *    та ключ командного рядка --log-level=<рівень>);
 *  - контекстні поля: session_id, модуль, платформа;
 *  - зручні макроси GM_LOG_* для лаконічного запису.
 *
 * Рівні (від найдрібнішого до найкритичнішого):
 *   TRACE → DEBUG → INFO → WARN → ERROR → CRITICAL
 */

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <memory>
#include <string>
#include <string_view>

namespace gm {

/**
 * @brief Конфігурація логера.
 *
 * Заповнюється при ініціалізації з env-змінних / аргументів CLI та
 * потенційно з файлу gesture_mouse.conf.
 */
struct LoggerConfig {
    std::string level        = "info";   ///< Мінімальний рівень (debug/info/warn/error/critical)
    std::string log_dir      = "logs";   ///< Директорія для файлових логів
    std::string log_file     = "gesture_mouse.log"; ///< Базове ім'я файлу
    std::size_t max_file_mb  = 5;        ///< Максимальний розмір файлу (МБ)
    std::size_t max_files    = 10;       ///< Кількість ротаційних файлів
    bool        console      = true;     ///< Виводити в консоль
    bool        async        = false;    ///< Асинхронний режим (для продакшн)
};

/**
 * @brief Ініціалізація системи логування.
 *
 * Повинна бути викликана один раз на початку main() після парсингу аргументів.
 * Якщо викликати кілька разів — повторний виклик безпечно ігнорується.
 *
 * @param cfg      Конфігурація логера.
 * @param session  Унікальний ідентифікатор сесії (генерується автоматично,
 *                 якщо порожній).
 */
void initLogger(const LoggerConfig& cfg = {}, std::string session = "");

/**
 * @brief Зупинка та скидання черги логів (flush).
 *
 * Обов'язково викликати перед виходом із програми.
 */
void shutdownLogger();

/**
 * @brief Повертає поточний глобальний логер.
 * @note  Nullptr, якщо initLogger() ще не викликали.
 */
std::shared_ptr<spdlog::logger> getLogger();

/**
 * @brief Повертає унікальний ідентифікатор поточної сесії.
 */
std::string_view getSessionId();

/**
 * @brief Визначає рівень spdlog з рядкового представлення.
 * @param level_str  Один із: trace, debug, info, warn/warning, error, critical.
 * @return           Відповідний spdlog::level::level_enum.
 * @throws std::invalid_argument  Якщо рядок не розпізнаний.
 */
spdlog::level::level_enum parseLoglevel(std::string_view level_str);

// ─── Зручні макроси ────────────────────────────────────────────────────────

#define GM_TRACE(...)    do { if (auto _l = ::gm::getLogger()) _l->trace   (__VA_ARGS__); } while(0)
#define GM_DEBUG(...)    do { if (auto _l = ::gm::getLogger()) _l->debug   (__VA_ARGS__); } while(0)
#define GM_INFO(...)     do { if (auto _l = ::gm::getLogger()) _l->info    (__VA_ARGS__); } while(0)
#define GM_WARN(...)     do { if (auto _l = ::gm::getLogger()) _l->warn    (__VA_ARGS__); } while(0)
#define GM_ERROR(...)    do { if (auto _l = ::gm::getLogger()) _l->error   (__VA_ARGS__); } while(0)
#define GM_CRITICAL(...) do { if (auto _l = ::gm::getLogger()) _l->critical(__VA_ARGS__); } while(0)

} // namespace gm
