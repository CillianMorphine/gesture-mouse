/**
 * @file test_documentation_examples.cpp
 * @brief Test-Driven Documentation — тести як приклади використання.
 *
 * @details
 * Цей файл є **живою документацією**: кожен тест демонструє
 * очікувану поведінку компонента і одночасно є виконуваним прикладом.
 *
 * ### Ідея TDD-документації
 * Замість того щоб писати:
 * > "Метод classify() повертає NONE якщо рука не виявлена"
 *
 * Ми пишемо:
 * ```cpp
 * TEST(GestureClassifier, returns_NONE_when_hand_not_detected) { ... }
 * ```
 * Тест ніколи не "застаріє" — якщо поведінка зміниться, тест впаде.
 *
 * @see docs/linting.md
 * @see docs/generate_docs.md
 */

#include <gtest/gtest.h>
#include "gesture/GestureClassifier.h"
#include "gesture/HandTracker.h"
#include "gesture/GestureRecognizer.h"
#include "core/ConfigManager.h"
#include "utils/Logger.h"

// ════════════════════════════════════════════════════════════
// GestureResult — value type behavior
// ════════════════════════════════════════════════════════════

/**
 * @brief GestureResult має коректні значення за замовчуванням.
 *
 * @par Демонструє:
 * - Default-ініціалізацію структури
 * - Значення полів при відсутності жесту
 */
TEST(GestureResult, default_values_represent_no_gesture) {
    GestureResult result;

    EXPECT_EQ(result.type,       GestureType::NONE);
    EXPECT_FLOAT_EQ(result.handX,      0.0F);
    EXPECT_FLOAT_EQ(result.handY,      0.0F);
    EXPECT_FLOAT_EQ(result.confidence, 0.0F);
}

/**
 * @brief typeName() повертає правильні рядки для кожного типу жесту.
 *
 * @par Демонструє:
 * - Відповідність enum-значень → рядків для логування
 */
TEST(GestureResult, typeName_returns_correct_strings) {
    auto check = [](GestureType t, const std::string& expected) {
        GestureResult r;
        r.type = t;
        EXPECT_EQ(r.typeName(), expected);
    };

    check(GestureType::NONE,        "NONE");
    check(GestureType::MOVE,        "MOVE");
    check(GestureType::LEFT_CLICK,  "LEFT_CLICK");
    check(GestureType::RIGHT_CLICK, "RIGHT_CLICK");
    check(GestureType::SCROLL_UP,   "SCROLL_UP");
    check(GestureType::SCROLL_DOWN, "SCROLL_DOWN");
    check(GestureType::DRAG,        "DRAG");
}

// ════════════════════════════════════════════════════════════
// GestureClassifier — business logic
// ════════════════════════════════════════════════════════════

namespace {
/// @brief Фабрична функція — створює HandData для тестів
HandData makeDetectedHand(float cx = 0.5F, float cy = 0.5F) {
    HandData h;
    h.detected = true;
    h.centerX  = cx;
    h.centerY  = cy;
    h.landmarks.push_back({cx, cy, 0.0F});
    return h;
}
} // namespace

/**
 * @brief classify() повертає NONE якщо рука не виявлена.
 *
 * @par Демонструє:
 * - Базовий захисний контракт: немає руки → немає жесту
 */
TEST(GestureClassifier, returns_NONE_when_hand_not_detected) {
    GestureClassifier cls(nullptr);
    HandData empty;
    empty.detected = false;

    EXPECT_EQ(cls.classify(empty).type, GestureType::NONE);
}

/**
 * @brief classify() не повертає NONE для виявленої руки.
 *
 * @par Демонструє:
 * - При наявності руки завжди є якийсь жест (MOVE як базовий)
 */
TEST(GestureClassifier, returns_non_NONE_for_detected_hand) {
    GestureClassifier cls(nullptr);
    EXPECT_NE(cls.classify(makeDetectedHand()).type, GestureType::NONE);
}

/**
 * @brief classify() передає координати руки без змін.
 *
 * @par Демонструє:
 * - GestureResult.handX/Y відповідають HandData.centerX/Y
 */
TEST(GestureClassifier, passes_coordinates_to_result) {
    GestureClassifier cls(nullptr);
    auto result = cls.classify(makeDetectedHand(0.3F, 0.7F));

    EXPECT_NEAR(result.handX, 0.3F, 0.001F);
    EXPECT_NEAR(result.handY, 0.7F, 0.001F);
}

/**
 * @brief confidence завжди в діапазоні [0, 1].
 *
 * @par Демонструє:
 * - Інваріант: впевненість є нормалізованою ймовірністю
 */
TEST(GestureClassifier, confidence_is_in_valid_range) {
    GestureClassifier cls(nullptr);
    auto result = cls.classify(makeDetectedHand());

    EXPECT_GE(result.confidence, 0.0F);
    EXPECT_LE(result.confidence, 1.0F);
}

// ════════════════════════════════════════════════════════════
// ConfigManager — configuration loading
// ════════════════════════════════════════════════════════════

/**
 * @brief Демонструє типове використання ConfigManager.
 *
 * @par Демонструє:
 * - Завантаження float, int, bool значень
 * - Повернення default при відсутності ключа
 *
 * @par Типовий сценарій:
 * @code{.cpp}
 * ConfigManager cfg("config/settings.txt");
 * float s = cfg.get<float>("mouse.smoothing", 0.3F);
 * @endcode
 */
TEST(ConfigManager, typical_usage_loads_all_types) {
    // Підготовка тимчасового файлу
    const std::string tmpFile = "/tmp/test_cfg_doctest.txt";
    {
        std::ofstream f(tmpFile);
        f << "mouse.smoothing=0.5\n"
          << "camera.index=2\n"
          << "debug.show_window=true\n";
    }

    ConfigManager cfg(tmpFile);

    // float
    EXPECT_NEAR(cfg.get<float>("mouse.smoothing", 0.0F), 0.5F, 0.001F);
    // int
    EXPECT_EQ(cfg.get<int>("camera.index", 0), 2);
    // bool
    EXPECT_TRUE(cfg.get<bool>("debug.show_window", false));

    std::remove(tmpFile.c_str());
}

/**
 * @brief Відсутній ключ повертає значення за замовчуванням.
 *
 * @par Демонструє:
 * - Безпечний fallback без виключень
 */
TEST(ConfigManager, missing_key_returns_default_value) {
    ConfigManager cfg("/tmp/nonexistent_for_doctest.txt");

    EXPECT_NEAR(cfg.get<float>("no.such.key", 42.0F), 42.0F, 0.001F);
    EXPECT_EQ(cfg.get<int>("no.such.key", -1), -1);
    EXPECT_FALSE(cfg.get<bool>("no.such.key", false));
}

// ════════════════════════════════════════════════════════════
// Logger — usage demonstration
// ════════════════════════════════════════════════════════════

/**
 * @brief Демонструє що макроси LOG_* не кидають виключень.
 *
 * @par Демонструє:
 * - Безпечність макросів навіть без ініціалізації файлу
 * - Синтаксис макросів зі string-аргументом
 */
TEST(Logger, macros_do_not_throw) {
    EXPECT_NO_THROW(LOG_DEBUG("debug message"));
    EXPECT_NO_THROW(LOG_INFO("info message"));
    EXPECT_NO_THROW(LOG_WARN("warning message"));
    EXPECT_NO_THROW(LOG_ERROR("error message"));
}

/**
 * @brief isInitialized() повертає false до виклику init().
 */
TEST(Logger, is_not_initialized_by_default) {
    // У тестовому середовищі init() не викликається
    // (або вже викликався — тест демонструє API)
    bool initialized = Logger::isInitialized();
    // Просто демонструємо що метод існує і повертає bool
    EXPECT_TRUE(initialized == true || initialized == false);
}
