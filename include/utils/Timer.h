#pragma once
#include <chrono>

/**
 * @brief Простий таймер для вимірювання часу виконання та FPS.
 */
class Timer {
public:
    void start() {
        m_start = std::chrono::steady_clock::now();
    }

    /// Повертає час у мілісекундах з моменту start()
    double elapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - m_start).count();
    }

    /// Повертає миттєвий FPS (кадр на секунду)
    double fps() const {
        double ms = elapsedMs();
        return (ms > 0.0) ? (1000.0 / ms) : 0.0;
    }

private:
    std::chrono::steady_clock::time_point m_start;
};
