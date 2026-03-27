/**
 * @file logger.cpp
 * @brief Реалізація системи логування GestureMouse.
 *
 * Особливості:
 *  - Два sink-и: кольорова консоль (stdout) + ротаційний файл.
 *  - Рівень логування визначається без перекомпіляції:
 *      1. змінна оточення GESTURE_LOG_LEVEL (найнижчий пріоритет);
 *      2. файл gesture_mouse.conf (ключ log_level=...);
 *      3. аргумент командного рядка --log-level=<рівень> (найвищий пріоритет).
 *  - Формат: [yyyy-mm-dd HH:MM:SS.mmm] [рівень] [session:<id>] повідомлення
 *  - Ротація: розмір файлу — 5 МБ, зберігається 10 останніх файлів.
 *  - session_id генерується один раз при запуску та пишеться в кожен рядок.
 */

#include "logger.hpp"

// spdlog — header-only бібліотека; підключаємо потрібні sink-и
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>

#include <chrono>
#include <cstdlib>   // std::getenv
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace gm {

// ─── Приватний стан ──────────────────────────────────────────────────────────
namespace {

std::shared_ptr<spdlog::logger> g_logger;
std::string                     g_session_id;
std::once_flag                  g_init_flag;

/**
 * @brief Генерує псевдовипадковий session_id у форматі hex (8 символів).
 *        Приклад: "a3f70c21"
 */
std::string generateSessionId() {
    std::mt19937 rng(
        static_cast<uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        )
    );
    std::uniform_int_distribution<uint32_t> dist(0x10000000u, 0xFFFFFFFFu);
    std::ostringstream oss;
    oss << std::hex << dist(rng);
    return oss.str();
}

/**
 * @brief Читає простий конфіг-файл формату key=value.
 *        Ігнорує рядки, що починаються на '#'.
 */
std::string readConfigKey(const std::string& path, const std::string& key) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto k = line.substr(0, eq);
        auto v = line.substr(eq + 1);
        // trim whitespace
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(k); trim(v);
        if (k == key) return v;
    }
    return {};
}

} // anonymous namespace

// ─── Публічний API ───────────────────────────────────────────────────────────

spdlog::level::level_enum parseLoglevel(std::string_view level_str) {
    // Перетворюємо в нижній регістр для порівняння
    std::string s(level_str);
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (s == "trace")                  return spdlog::level::trace;
    if (s == "debug")                  return spdlog::level::debug;
    if (s == "info")                   return spdlog::level::info;
    if (s == "warn" || s == "warning") return spdlog::level::warn;
    if (s == "error")                  return spdlog::level::err;
    if (s == "critical")               return spdlog::level::critical;

    throw std::invalid_argument(
        "Невідомий рівень логування: '" + std::string(level_str) +
        "'. Допустимі: trace, debug, info, warn, error, critical."
    );
}

void initLogger(const LoggerConfig& cfg, std::string session) {
    std::call_once(g_init_flag, [&] {
        // 1. Session ID
        g_session_id = session.empty() ? generateSessionId() : std::move(session);

        // 2. Визначення рівня (пріоритет: cfg > конфіг-файл > env)
        std::string level_str = cfg.level; // може прийти вже заповненим з CLI

        // Якщо рівень ще не визначено CLI — перевіряємо gesture_mouse.conf
        if (level_str == "info") { // "info" — значення за замовчуванням, шукаємо далі
            auto from_file = readConfigKey("gesture_mouse.conf", "log_level");
            if (!from_file.empty()) level_str = from_file;
        }

        // Перевіряємо змінну оточення (найнижчий пріоритет)
        if (const char* env = std::getenv("GESTURE_LOG_LEVEL")) {
            if (level_str == "info") // якщо ще не перевизначено
                level_str = env;
        }

        auto log_level = spdlog::level::info; // fallback
        try {
            log_level = parseLoglevel(level_str);
        } catch (...) {
            // Невірне значення — залишаємо info, попередимо пізніше
        }

        // 3. Формат рядка лога
        // [2024-06-15 14:32:07.123] [info ] [session:a3f70c21] Повідомлення
        std::string pattern =
            "[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [session:" + g_session_id + "] %v";

        // 4. Sink-и
        std::vector<spdlog::sink_ptr> sinks;

        // 4a. Консоль (кольоровий вивід)
        if (cfg.console) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(log_level);
            console_sink->set_pattern(pattern);
            sinks.push_back(console_sink);
        }

        // 4b. Файловий sink з ротацією за розміром
        try {
            std::filesystem::create_directories(cfg.log_dir);
            auto file_path = cfg.log_dir + "/" + cfg.log_file;
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                file_path,
                cfg.max_file_mb * 1024 * 1024, // bytes
                cfg.max_files
            );
            file_sink->set_level(spdlog::level::trace); // файл пише всі рівні
            file_sink->set_pattern(pattern);
            sinks.push_back(file_sink);
        } catch (const std::exception& ex) {
            // Якщо не вдалося відкрити файл — продовжуємо лише з консоллю
            fprintf(stderr, "[logger] Не вдалося відкрити файл логу: %s\n", ex.what());
        }

        // 5. Створення логера з кількома sink-ами
        g_logger = std::make_shared<spdlog::logger>("gesture_mouse",
                                                     sinks.begin(), sinks.end());
        g_logger->set_level(log_level);
        g_logger->flush_on(spdlog::level::err); // авто-flush при помилках

        spdlog::register_logger(g_logger);
        spdlog::set_default_logger(g_logger);

        // 6. Перший запис — початок сесії
        g_logger->info("══════════════════════════════════════════════════════");
        g_logger->info("GestureMouse запущено. Session: {}", g_session_id);
        g_logger->info("Рівень логування: {} | Платформа: {}",
#if defined(_WIN32)
            level_str, "Windows"
#else
            level_str, "Linux"
#endif
        );
        g_logger->info("──────────────────────────────────────────────────────");
    });
}

void shutdownLogger() {
    if (g_logger) {
        g_logger->info("──────────────────────────────────────────────────────");
        g_logger->info("GestureMouse завершено. Session: {}", g_session_id);
        g_logger->info("══════════════════════════════════════════════════════");
        g_logger->flush();
        spdlog::shutdown();
        g_logger.reset();
    }
}

std::shared_ptr<spdlog::logger> getLogger()  { return g_logger;      }
std::string_view                getSessionId(){ return g_session_id;  }

} // namespace gm
