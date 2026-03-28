#pragma once
/**
 * @file profiler.hpp
 * @brief Вбудована система профілювання та метрик продуктивності GestureMouse.
 *
 * Надає:
 *  - ScopedTimer    — RAII-таймер для вимірювання часу блоку коду;
 *  - PerfCounter    — накопичувач статистики (avg/min/max/count) для операцій;
 *  - FrameProfiler  — профілювання кожного кадру обробки відео;
 *  - PerfRegistry   — глобальний реєстр усіх лічильників;
 *  - макрос GM_PROFILE_SCOPE(name) — найзручніший спосіб профілювання.
 *
 * Використання:
 * @code
 *   // Профілювання довільного блоку:
 *   {
 *       GM_PROFILE_SCOPE("hsv_segmentation");
 *       cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
 *       cv::inRange(hsv, lower, upper, mask);
 *   }
 *
 *   // Звіт у лог (кожні N кадрів):
 *   gm::PerfRegistry::instance().logReport();
 * @endcode
 *
 * @note  Профілювання вмикається лише при компіляції з GM_PROFILING_ENABLED.
 *        У release-білдах усі макроси розгортаються в порожні вирази.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gm {

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Micros    = std::chrono::microseconds;

// ─── PerfCounter ─────────────────────────────────────────────────────────────

/**
 * @brief Потокобезпечний акумулятор статистики для одного лічильника.
 *
 * Зберігає: кількість вимірів, суму, мінімум, максимум.
 * Розраховує: середнє, P95 (приблизне на базі sliding window).
 */
class PerfCounter {
public:
    explicit PerfCounter(std::string name) : name_(std::move(name)) {}

    /** @brief Додає одне вимірювання (мікросекунди). */
    void record(int64_t us) {
        std::lock_guard<std::mutex> lk(mu_);
        ++count_;
        sum_us_  += us;
        min_us_   = std::min(min_us_, us);
        max_us_   = std::max(max_us_, us);

        // Sliding window для P95
        window_.push_back(us);
        if (window_.size() > kWindowSize) window_.erase(window_.begin());
    }

    struct Stats {
        std::string name;
        uint64_t    count    = 0;
        double      avg_ms   = 0;
        double      min_ms   = 0;
        double      max_ms   = 0;
        double      p95_ms   = 0;
        double      total_ms = 0;
    };

    Stats stats() const {
        std::lock_guard<std::mutex> lk(mu_);
        if (count_ == 0) return { name_ };
        Stats s;
        s.name     = name_;
        s.count    = count_;
        s.avg_ms   = static_cast<double>(sum_us_) / count_ / 1000.0;
        s.min_ms   = min_us_ / 1000.0;
        s.max_ms   = max_us_ / 1000.0;
        s.total_ms = sum_us_ / 1000.0;

        // P95: сортуємо вікно
        if (!window_.empty()) {
            auto w = window_;
            std::sort(w.begin(), w.end());
            s.p95_ms = w[static_cast<size_t>(w.size() * 0.95)] / 1000.0;
        }
        return s;
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mu_);
        count_  = 0; sum_us_ = 0;
        min_us_ = std::numeric_limits<int64_t>::max();
        max_us_ = 0;
        window_.clear();
    }

    std::string_view name() const { return name_; }

private:
    static constexpr std::size_t kWindowSize = 200;

    std::string            name_;
    mutable std::mutex     mu_;
    uint64_t               count_  = 0;
    int64_t                sum_us_ = 0;
    int64_t                min_us_ = std::numeric_limits<int64_t>::max();
    int64_t                max_us_ = 0;
    std::vector<int64_t>   window_;
};

// ─── ScopedTimer ─────────────────────────────────────────────────────────────

/**
 * @brief RAII-таймер: фіксує час від створення до знищення об'єкта.
 *
 * При знищенні автоматично записує результат у PerfCounter.
 */
class ScopedTimer {
public:
    explicit ScopedTimer(PerfCounter& counter)
        : counter_(counter), start_(Clock::now()) {}

    ~ScopedTimer() {
        auto elapsed = std::chrono::duration_cast<Micros>(Clock::now() - start_);
        counter_.record(elapsed.count());
    }

    // Не копіювати/переміщувати
    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    PerfCounter& counter_;
    TimePoint    start_;
};

// ─── PerfRegistry ─────────────────────────────────────────────────────────────

/**
 * @brief Глобальний реєстр лічильників продуктивності.
 *
 * Singleton. Лічильники реєструються автоматично при першому використанні.
 */
class PerfRegistry {
public:
    static PerfRegistry& instance() {
        static PerfRegistry reg;
        return reg;
    }

    /** @brief Отримати (або створити) лічильник за ім'ям. */
    PerfCounter& get(std::string_view name) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = counters_.find(std::string(name));
        if (it == counters_.end()) {
            auto [ins, _] = counters_.emplace(
                std::string(name),
                std::make_unique<PerfCounter>(std::string(name))
            );
            return *ins->second;
        }
        return *it->second;
    }

    /** @brief Вивести зведений звіт у лог (spdlog). */
    void logReport() const;

    /** @brief Скинути всі лічильники. */
    void resetAll() {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& [_, c] : counters_) c->reset();
    }

    /** @brief Зберегти звіт у CSV-файл. */
    void saveCSV(std::string_view path) const;

private:
    PerfRegistry() = default;
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::unique_ptr<PerfCounter>> counters_;
};

// ─── FrameProfiler ────────────────────────────────────────────────────────────

/**
 * @brief Профілювальник кадру: вимірює час кожного етапу обробки.
 *
 * Використання:
 * @code
 *   FrameProfiler fp;
 *   fp.begin("capture");   // ... cap.read(frame) ...
 *   fp.begin("hsv");       // ... cvtColor ...
 *   fp.begin("contours");  // ... findContours ...
 *   fp.end();              // записує всі вимірювання
 * @endcode
 */
class FrameProfiler {
public:
    void begin(std::string_view stage) {
        if (!current_.empty()) flush();
        current_ = std::string(stage);
        t0_      = Clock::now();
    }

    void end() { flush(); }

    /** @brief Поточний FPS (1 / час_останнього_кадру). */
    double fps() const {
        if (last_frame_us_ == 0) return 0.0;
        return 1'000'000.0 / last_frame_us_;
    }

private:
    void flush() {
        if (current_.empty()) return;
        auto us = std::chrono::duration_cast<Micros>(Clock::now() - t0_).count();
        PerfRegistry::instance().get("frame." + current_).record(us);
        last_frame_us_ += us;
        if (current_ == "input") { // останній етап
            PerfRegistry::instance().get("frame.total").record(last_frame_us_);
            last_frame_us_ = 0;
        }
        current_.clear();
    }

    std::string current_;
    TimePoint   t0_;
    int64_t     last_frame_us_ = 0;
};

// ─── Макроси профілювання ────────────────────────────────────────────────────

#ifdef GM_PROFILING_ENABLED
  /**
   * @brief Профілює поточний scope. Результат записується у PerfRegistry.
   * @param name  Рядковий ідентифікатор (без пробілів).
   */
  #define GM_PROFILE_SCOPE(name) \
      ::gm::ScopedTimer _gm_timer_##__LINE__( \
          ::gm::PerfRegistry::instance().get(name))

  #define GM_PROFILE_REPORT() \
      ::gm::PerfRegistry::instance().logReport()

  #define GM_PROFILE_CSV(path) \
      ::gm::PerfRegistry::instance().saveCSV(path)
#else
  #define GM_PROFILE_SCOPE(name)  (void)0
  #define GM_PROFILE_REPORT()     (void)0
  #define GM_PROFILE_CSV(path)    (void)0
#endif

} // namespace gm
