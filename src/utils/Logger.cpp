// ============================================================
// Logger.cpp
// Fix: static members defined here (not inline in header)
// ============================================================
#include "utils/Logger.h"

// Static member definitions
std::ofstream Logger::s_file;
std::mutex    Logger::s_mutex;

void Logger::init(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_file.open(filePath, std::ios::app);
    if (!s_file.is_open()) {
        std::cerr << "[Logger] Cannot open log file: " << filePath << "\n";
    }
}

void Logger::log(LogLevel level, const std::string& message,
                 const char* file, int line) {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::ostringstream oss;
    oss << "[" << timestamp() << "] "
        << "[" << levelToString(level) << "] "
        << message
        << " (" << file << ":" << line << ")";

    const std::string entry = oss.str();  // Fix: const (readability-const-return-type)
    std::cout << entry << "\n";
    if (s_file.is_open()) {
        s_file << entry << "\n";
    }
}

std::string Logger::levelToString(LogLevel l) {
    // Fix: switch on scoped enum (modernize-use-using)
    switch (l) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
    }
    return "?????";  // Fix: removed default — compiler warns on missing enum cases
}

std::string Logger::timestamp() {
    const auto now = std::chrono::system_clock::now();  // Fix: const
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
