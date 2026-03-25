#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static void init(const std::string& filePath);
    static void log(LogLevel level, const std::string& message,
                    const char* file, int line);

private:
    static std::string levelToString(LogLevel l);
    static std::string timestamp();

    static inline std::ofstream s_file;
    static inline std::mutex    s_mutex;
};

// Зручні макроси
#define LOG_DEBUG(msg) Logger::log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::log(LogLevel::INFO,  msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::log(LogLevel::WARN,  msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::log(LogLevel::ERROR, msg, __FILE__, __LINE__)
