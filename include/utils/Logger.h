#pragma once
// ============================================================
// Logger.h — Lightweight thread-safe logger
// Fix 1: replaced multiple static inline members with proper
//         initialization (cppcoreguidelines-interfaces-global-init)
// Fix 2: added [[nodiscard]] where appropriate
// Fix 3: used scoped enum instead of plain enum
// ============================================================
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// Fix 4: scoped enum (modernize-use-using, cppcoreguidelines-enum-initial-value)
enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
};

class Logger {
public:
    // Fix 5: deleted copy/move (cppcoreguidelines-special-member-functions)
    Logger()                           = delete;
    Logger(const Logger&)              = delete;
    Logger& operator=(const Logger&)   = delete;
    Logger(Logger&&)                   = delete;
    Logger& operator=(Logger&&)        = delete;

    static void init(const std::string& filePath);
    static void log(LogLevel level, const std::string& message,
                    const char* file, int line);

    // Fix 6: [[nodiscard]] on pure query (modernize-use-nodiscard — manually)
    [[nodiscard]] static bool isInitialized() noexcept { return s_file.is_open(); }

private:
    [[nodiscard]] static std::string levelToString(LogLevel l);
    [[nodiscard]] static std::string timestamp();

    static std::ofstream s_file;   // Fix 7: removed inline — defined in .cpp
    static std::mutex    s_mutex;
};

// Convenience macros — wrap in do/while to avoid dangling-else issues
// Fix 8: added do/while(false) (bugprone-macro-parentheses)
#define LOG_DEBUG(msg) do { Logger::log(LogLevel::DEBUG, (msg), __FILE__, __LINE__); } while(false)
#define LOG_INFO(msg)  do { Logger::log(LogLevel::INFO,  (msg), __FILE__, __LINE__); } while(false)
#define LOG_WARN(msg)  do { Logger::log(LogLevel::WARN,  (msg), __FILE__, __LINE__); } while(false)
#define LOG_ERROR(msg) do { Logger::log(LogLevel::ERROR, (msg), __FILE__, __LINE__); } while(false)
