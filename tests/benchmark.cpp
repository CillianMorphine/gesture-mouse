/**
 * @file benchmark.cpp
 * @brief Мікробенчмарки продуктивності GestureMouse.
 *
 * Вимірює час кожного етапу конвеєра обробки без реальної камери.
 * Використовує синтетичні кадри (cv::Mat із заповненням кольором).
 *
 * Запуск:
 *   ./gesture_mouse_bench [--frames=N] [--width=W] [--height=H]
 *
 * Виводить:
 *   - таблицю середніх/мін/макс часів для кожного етапу;
 *   - CSV-файл benchmark_results/results.csv;
 *   - розрахунковий FPS (1000 / avg_total_ms).
 *
 * Тестові сценарії:
 *   Scenario A — малий кадр  (320×240),   100 ітерацій
 *   Scenario B — середній   (640×480),   500 ітерацій
 *   Scenario C — великий    (1280×720), 1000 ітерацій
 */

// ─── Заглушки OpenCV для компіляції без реальної бібліотеки ──────────────────
// У реальному проекті замінити на: #include <opencv2/opencv.hpp>
// Нижче — мінімальні стаби для ілюстрації структури коду.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ─── Мінімальна заглушка Mat (замість cv::Mat) ───────────────────────────────
struct FakeMat {
    int rows, cols, channels;
    std::vector<uint8_t> data;
    FakeMat() : rows(0), cols(0), channels(0) {}
    FakeMat(int r, int c, int ch = 3)
        : rows(r), cols(c), channels(ch), data(r * c * ch, 0) {}
    bool empty() const { return data.empty(); }
};

// Фейкові версії операцій OpenCV (замінити реальними cv:: функціями)
namespace fake_cv {
    using Mat = FakeMat;
    static void cvtColor(const Mat& src, Mat& dst, int) {
        dst = Mat(src.rows, src.cols, 3);
        // Симуляція: просто копіюємо
        std::copy(src.data.begin(), src.data.end(), dst.data.begin());
        // Реальна затримка HSV-конвертації для 640x480 ≈ 1.2 мс
        volatile int64_t sink = 0;
        for (int i = 0; i < src.rows * src.cols; ++i) sink += src.data[i % src.data.size()];
    }
    static void inRange(const Mat& src, Mat& dst) {
        dst = Mat(src.rows, src.cols, 1);
        for (size_t i = 0; i < src.data.size() / 3; ++i)
            dst.data[i] = (src.data[i*3] > 50 && src.data[i*3] < 200) ? 255 : 0;
    }
    static void morphology(Mat& m) {
        // Симуляція морфологічної обробки (erode + dilate)
        volatile uint64_t s = 0;
        for (auto b : m.data) s += b;
        (void)s;
    }
    static std::vector<std::vector<std::pair<int,int>>> findContours(const Mat& m) {
        // Спрощена симуляція: повертає 1 контур
        (void)m;
        std::vector<std::pair<int,int>> c;
        for (int i = 0; i < 100; ++i) c.push_back({i, i*2});
        return { c };
    }
    static std::pair<int,int> convexHull(const std::vector<std::pair<int,int>>& pts) {
        // Спрощена симуляція
        volatile int s = 0;
        for (auto [x,y] : pts) s += x + y;
        return {s % 640, s % 480};
    }
}

// ─── Таймер ───────────────────────────────────────────────────────────────────
using Clock = std::chrono::steady_clock;
using Us    = std::chrono::microseconds;

static int64_t now_us() {
    return std::chrono::duration_cast<Us>(Clock::now().time_since_epoch()).count();
}

// ─── Статистика ───────────────────────────────────────────────────────────────
struct StageStats {
    std::string            name;
    std::vector<int64_t>   samples_us;

    void record(int64_t us) { samples_us.push_back(us); }

    double avg_ms() const {
        if (samples_us.empty()) return 0;
        return static_cast<double>(
            std::accumulate(samples_us.begin(), samples_us.end(), 0LL)
        ) / samples_us.size() / 1000.0;
    }
    double min_ms() const {
        return *std::min_element(samples_us.begin(), samples_us.end()) / 1000.0;
    }
    double max_ms() const {
        return *std::max_element(samples_us.begin(), samples_us.end()) / 1000.0;
    }
    double p95_ms() const {
        auto v = samples_us;
        std::sort(v.begin(), v.end());
        return v[static_cast<size_t>(v.size() * 0.95)] / 1000.0;
    }
};

// ─── Основний конвеєр ─────────────────────────────────────────────────────────

struct BenchScenario {
    std::string label;
    int width, height, frames;
};

static void runScenario(const BenchScenario& sc, std::vector<StageStats>& out) {
    using namespace fake_cv;
    std::cout << "\n[Scenario " << sc.label << "] "
              << sc.width << "×" << sc.height << ", " << sc.frames << " кадрів\n";

    const std::vector<std::string> stages = {
        "hsv_convert", "inrange_mask", "morphology", "find_contours",
        "convex_hull", "gesture_classify", "ema_smooth"
    };

    std::vector<StageStats> stats;
    for (auto& s : stages) stats.push_back({s});

    // Синтетичний кадр
    Mat frame(sc.height, sc.width, 3);
    // Заповнення тілесним кольором (HSV-подібне)
    for (size_t i = 0; i < frame.data.size(); i += 3) {
        frame.data[i]   = 170; // H ≈ тілесний
        frame.data[i+1] = 120; // S
        frame.data[i+2] = 180; // V
    }

    Mat hsv, mask;
    float ema_x = sc.width / 2.0f, ema_y = sc.height / 2.0f;
    const float alpha = 0.3f; // EMA-коефіцієнт

    for (int f = 0; f < sc.frames; ++f) {
        // ── 1. HSV-конвертація ──────────────────────────────────────────────
        {
            auto t0 = now_us();
            cvtColor(frame, hsv, 40 /* COLOR_BGR2HSV */);
            stats[0].record(now_us() - t0);
        }

        // ── 2. inRange (виділення маски тілесного кольору) ─────────────────
        {
            auto t0 = now_us();
            inRange(hsv, mask);
            stats[1].record(now_us() - t0);
        }

        // ── 3. Морфологічна обробка (erode + dilate) ───────────────────────
        {
            auto t0 = now_us();
            morphology(mask);
            stats[2].record(now_us() - t0);
        }

        // ── 4. Пошук контурів ──────────────────────────────────────────────
        auto t0_c = now_us();
        auto contours = findContours(mask);
        stats[3].record(now_us() - t0_c);

        // ── 5. Опукла оболонка ─────────────────────────────────────────────
        auto t0_h = now_us();
        std::pair<int,int> centroid = {0,0};
        if (!contours.empty()) centroid = convexHull(contours[0]);
        stats[4].record(now_us() - t0_h);

        // ── 6. Класифікація жесту ──────────────────────────────────────────
        {
            auto t0 = now_us();
            // Спрощена класифікація: підрахунок «пальців» за кутами
            volatile int fingers = 0;
            if (!contours.empty())
                fingers = static_cast<int>(contours[0].size()) / 20;
            stats[5].record(now_us() - t0);
        }

        // ── 7. EMA-згладжування координат ─────────────────────────────────
        {
            auto t0 = now_us();
            ema_x = alpha * centroid.first  + (1 - alpha) * ema_x;
            ema_y = alpha * centroid.second + (1 - alpha) * ema_y;
            stats[6].record(now_us() - t0);
        }

        // Прогрес
        if ((f + 1) % (sc.frames / 5) == 0)
            std::cout << "  " << (f + 1) << "/" << sc.frames << " кадрів...\r" << std::flush;
    }
    std::cout << "\n";

    out = std::move(stats);
}

// ─── Вивід та збереження CSV ─────────────────────────────────────────────────

static void printTable(const std::string& label,
                        const std::vector<StageStats>& stats) {
    std::cout << "\n┌─────────────────────────────────────────────────────────┐\n";
    std::cout << "│  " << label
              << std::string(55 - label.size(), ' ') << "│\n";
    std::cout << "├───────────────────┬────────┬────────┬────────┬────────┤\n";
    std::cout << "│ Операція          │ Avg ms │ Min ms │ Max ms │ P95 ms │\n";
    std::cout << "├───────────────────┼────────┼────────┼────────┼────────┤\n";

    double total_avg = 0;
    for (auto& s : stats) {
        std::cout << "│ " << std::left << std::setw(17) << s.name << " │ "
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(6) << s.avg_ms() << " │ "
                  << std::setw(6) << s.min_ms() << " │ "
                  << std::setw(6) << s.max_ms() << " │ "
                  << std::setw(6) << s.p95_ms() << " │\n";
        total_avg += s.avg_ms();
    }
    std::cout << "├───────────────────┼────────┼────────┼────────┼────────┤\n";
    std::cout << "│ TOTAL             │ "
              << std::setw(6) << total_avg << " │        │        │        │\n";
    double fps = total_avg > 0 ? 1000.0 / total_avg : 0;
    std::cout << "│ Est. FPS          │ "
              << std::setw(6) << std::setprecision(1) << fps
              << " │        │        │        │\n";
    std::cout << "└───────────────────┴────────┴────────┴────────┴────────┘\n";
}

static void saveCSV(const std::string& path,
                    const std::string& scenario,
                    const std::vector<StageStats>& stats) {
    std::ofstream f(path, std::ios::app);
    for (auto& s : stats) {
        f << scenario << "," << s.name << ","
          << std::fixed << std::setprecision(4)
          << s.avg_ms() << "," << s.min_ms() << ","
          << s.max_ms() << "," << s.p95_ms() << "\n";
    }
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     GestureMouse Performance Benchmark v1.0              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::filesystem::create_directories("benchmark_results");
    const std::string csv_path = "benchmark_results/results.csv";

    // Заголовок CSV
    {
        std::ofstream f(csv_path);
        f << "scenario,operation,avg_ms,min_ms,max_ms,p95_ms\n";
    }

    // Три сценарії
    std::vector<BenchScenario> scenarios = {
        { "A_small",  320,  240,  100 },
        { "B_medium", 640,  480,  500 },
        { "C_large", 1280,  720, 1000 },
    };

    for (auto& sc : scenarios) {
        std::vector<StageStats> stats;
        runScenario(sc, stats);
        printTable("Scenario " + sc.label, stats);
        saveCSV(csv_path, sc.label, stats);
    }

    std::cout << "\nCSV збережено: " << csv_path << "\n";
    return 0;
}
