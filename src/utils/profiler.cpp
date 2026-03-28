/**
 * @file profiler.cpp
 * @brief Реалізація системи профілювання GestureMouse.
 */

#include "profiler.hpp"
#include "logger.hpp"  // GM_INFO, GM_DEBUG

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace gm {

// ─── PerfRegistry::logReport ─────────────────────────────────────────────────

void PerfRegistry::logReport() const {
    std::lock_guard<std::mutex> lk(mu_);

    // Зібрати та відсортувати за avg (спадання)
    std::vector<PerfCounter::Stats> all;
    all.reserve(counters_.size());
    for (auto& [_, c] : counters_) all.push_back(c->stats());
    std::sort(all.begin(), all.end(),
              [](const auto& a, const auto& b){ return a.avg_ms > b.avg_ms; });

    GM_INFO("╔══════════════════════════════════════════════════════════════════╗");
    GM_INFO("║          GestureMouse — Performance Report                      ║");
    GM_INFO("╠══════════════════╦═══════╦══════════╦══════════╦══════════╦═════╣");
    GM_INFO("║ Operation        ║ Count ║  Avg(ms) ║  Min(ms) ║  Max(ms) ║ P95 ║");
    GM_INFO("╠══════════════════╬═══════╬══════════╬══════════╬══════════╬═════╣");

    for (const auto& s : all) {
        if (s.count == 0) continue;
        GM_INFO("║ {:<16} ║ {:>5} ║ {:>8.3f} ║ {:>8.3f} ║ {:>8.3f} ║{:>4.1f}║",
                s.name.substr(0, 16),
                s.count,
                s.avg_ms, s.min_ms, s.max_ms, s.p95_ms);
    }
    GM_INFO("╚══════════════════╩═══════╩══════════╩══════════╩══════════╩═════╝");
}

// ─── PerfRegistry::saveCSV ───────────────────────────────────────────────────

void PerfRegistry::saveCSV(std::string_view path) const {
    std::lock_guard<std::mutex> lk(mu_);
    std::ofstream f(std::string(path));
    if (!f.is_open()) {
        GM_WARN("[profiler] Не вдалося зберегти CSV: {}", path);
        return;
    }

    f << "operation,count,avg_ms,min_ms,max_ms,p95_ms,total_ms\n";
    for (auto& [_, c] : counters_) {
        auto s = c->stats();
        f << std::fixed << std::setprecision(4)
          << s.name    << ","
          << s.count   << ","
          << s.avg_ms  << ","
          << s.min_ms  << ","
          << s.max_ms  << ","
          << s.p95_ms  << ","
          << s.total_ms << "\n";
    }
    GM_INFO("[profiler] Звіт збережено: {}", path);
}

} // namespace gm
