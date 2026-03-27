#pragma once
/**
 * @file GestureRecognizer.h
 * @brief Фасад модуля розпізнавання жестів руки.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * Цей модуль реалізує **конвеєр розпізнавання** у три кроки:
 *
 * @dot
 * digraph pipeline {
 *   rankdir=LR;
 *   node [shape=box, style=filled, fillcolor="#1a2540", color="#4a9eff",
 *         fontcolor=white, fontname=Helvetica];
 *   A [label="cv::Mat\n(raw frame)"];
 *   B [label="HandTracker\n(HSV segmentation)"];
 *   C [label="GestureClassifier\n(finger counting)"];
 *   D [label="GestureResult\n(type + coords)"];
 *   A -> B -> C -> D;
 * }
 * @enddot
 *
 * ### Вибір алгоритму
 * Поточна реалізація використовує **HSV-сегментацію тілесного кольору**
 * як компроміс між точністю та швидкістю без потреби в GPU чи ML-моделі.
 *
 * | Підхід | Точність | Швидкість | Залежності |
 * |--------|----------|-----------|------------|
 * | HSV-сегментація (поточний) | Середня | Висока | OpenCV |
 * | MediaPipe Hands | Висока | Висока | MediaPipe, Python/C++ |
 * | CNN (власна модель) | Дуже висока | Середня | TensorFlow/ONNX |
 *
 * @see HandTracker
 * @see GestureClassifier
 */

#include <memory>
#include <string>
#include <opencv2/core/mat.hpp>

/**
 * @brief Перелік усіх розпізнаваних типів жестів.
 *
 * @details
 * Відповідність жестів → дій миші:
 *
 * | Значення | Жест | Дія миші |
 * |----------|------|----------|
 * | NONE | Рука не виявлена | — |
 * | MOVE | Відкрита долоня (4+ пальців) | Переміщення курсору |
 * | LEFT_CLICK | Вказівний палець | Клік лівою кнопкою |
 * | RIGHT_CLICK | Два пальці (V) | Клік правою кнопкою |
 * | SCROLL_UP | Рух угору з долонею | Прокрутка вгору |
 * | SCROLL_DOWN | Рух вниз з долонею | Прокрутка вниз |
 * | DRAG | Стиснутий кулак у русі | Перетягування |
 *
 * @note Значення enum є стабільними — їх можна зберігати у конфіг-файлах.
 */
enum class GestureType : uint8_t {
    NONE        = 0, ///< Жест не виявлено або рука поза кадром
    MOVE        = 1, ///< Переміщення курсору (відкрита долоня)
    LEFT_CLICK  = 2, ///< Клік лівою кнопкою (1 палець)
    RIGHT_CLICK = 3, ///< Клік правою кнопкою (2 пальці — V)
    SCROLL_UP   = 4, ///< Прокрутка вгору
    SCROLL_DOWN = 5, ///< Прокрутка вниз
    DRAG        = 6, ///< Перетягування (кулак + рух)
};

/**
 * @brief Результат розпізнавання одного кадру відеопотоку.
 *
 * @details
 * Структура є **value type** (копіюється дешево).
 * Координати нормалізовані до діапазону [0.0, 1.0] відносно
 * розмірів кадру камери — MouseController перетворює їх у
 * піксельні координати екрана.
 *
 * @par Приклад використання:
 * @code{.cpp}
 * GestureResult result = recognizer.process(frame);
 * if (result.type == GestureType::MOVE && result.confidence > 0.7F) {
 *     mouseCtrl.applyGesture(result);
 * }
 * @endcode
 */
struct GestureResult {
    GestureType type{GestureType::NONE}; ///< Тип розпізнаного жесту
    float handX{0.0F};      ///< Нормалізована X-координата центру руки [0..1]
    float handY{0.0F};      ///< Нормалізована Y-координата центру руки [0..1]
    float confidence{0.0F}; ///< Впевненість класифікатора [0..1], 1.0 = максимум

    /**
     * @brief Повертає рядкову назву типу жесту.
     *
     * @details Зручно для логування та debug-виводу:
     * @code{.cpp}
     * LOG_DEBUG("Gesture: " + result.typeName());
     * @endcode
     *
     * @return Рядок у верхньому регістрі, наприклад `"LEFT_CLICK"`,
     *         або `"NONE"` якщо жест не виявлено.
     */
    [[nodiscard]] std::string typeName() const;
};

// Forward declarations
class ConfigManager;
class HandTracker;
class GestureClassifier;

/**
 * @brief Фасад модуля розпізнавання — координує HandTracker і GestureClassifier.
 *
 * @details
 * GestureRecognizer є **єдиною публічною точкою входу** до підсистеми CV.
 * Application взаємодіє лише з цим класом, не знаючи деталей реалізації
 * HandTracker або GestureClassifier.
 *
 * ### Патерн Facade
 * ```
 * Application
 *     │
 *     └──► GestureRecognizer::process(frame)
 *               ├──► HandTracker::detect(frame)      → HandData
 *               └──► GestureClassifier::classify(hand) → GestureResult
 * ```
 *
 * ### Налаштування через ConfigManager
 * | Ключ | Тип | За замовч. | Опис |
 * |------|-----|------------|------|
 * | `gesture.confidence` | float | 0.6 | Мін. впевненість для підтвердження |
 * | `camera.index` | int | 0 | Індекс камери для OpenCV |
 * | `debug.show_window` | bool | false | Показувати debug-кадр |
 */
class GestureRecognizer {
public:
    /**
     * @brief Конструктор. Ініціалізує HandTracker і GestureClassifier.
     *
     * @param config Вказівник на менеджер конфігурації (не власник, не nullptr).
     *
     * @throws std::invalid_argument якщо config == nullptr.
     * @throws std::runtime_error якщо не вдалося ініціалізувати трекер.
     *
     * @pre config != nullptr && config->isLoaded()
     */
    explicit GestureRecognizer(ConfigManager* config);

    /**
     * @brief Деструктор. Закриває debug-вікно якщо воно відкрите.
     */
    ~GestureRecognizer();

    /// @name Некопійовані / переміщувані операції
    /// @{
    GestureRecognizer(const GestureRecognizer&)            = delete;
    GestureRecognizer& operator=(const GestureRecognizer&) = delete;
    GestureRecognizer(GestureRecognizer&&)                 = default;
    GestureRecognizer& operator=(GestureRecognizer&&)      = default;
    /// @}

    /**
     * @brief Обробляє один кадр і повертає розпізнаний жест.
     *
     * @details
     * **Алгоритм обробки:**
     * 1. `HandTracker::detect(frame)` → знаходить контур руки через HSV-маску
     * 2. Якщо рука не виявлена — повертає `GestureResult{GestureType::NONE}`
     * 3. `GestureClassifier::classify(handData)` → визначає тип жесту
     * 4. Якщо ввімкнено debug-вікно — накладає landmarks на кадр
     *
     * **Продуктивність:** типовий час виконання < 5 мс на CPU (Intel i5).
     *
     * @param frame Кадр BGR з камери (CV_8UC3). Не модифікується.
     *
     * @return GestureResult з типом жесту, координатами та впевненістю.
     *         Якщо frame порожній — повертає NONE без помилок.
     *
     * @note Метод **не є потокобезпечним** — викликати тільки з одного потоку.
     *
     * @par Приклад:
     * @code{.cpp}
     * cv::VideoCapture cap(0);
     * cv::Mat frame;
     * while (cap.read(frame)) {
     *     auto result = recognizer.process(frame);
     *     if (result.type != GestureType::NONE)
     *         LOG_INFO("Gesture: " + result.typeName());
     * }
     * @endcode
     */
    [[nodiscard]] GestureResult process(const cv::Mat& frame);

    /**
     * @brief Вмикає або вимикає debug-вікно з накладеними landmarks.
     *
     * @details
     * При увімкненні відкривається вікно `"GestureMouse Debug"` з:
     * - Контуром виявленої руки (зелений)
     * - Центром мас (синій кружок)
     * - Назвою розпізнаного жесту (білий текст)
     *
     * При вимкненні вікно закривається через `cv::destroyWindow()`.
     *
     * @param enabled true — показати вікно, false — закрити.
     *
     * @note noexcept — помилки OpenCV при роботі з вікнами ігноруються.
     */
    void setDebugView(bool enabled) noexcept;

private:
    std::unique_ptr<HandTracker>       m_tracker;    ///< Детектор руки
    std::unique_ptr<GestureClassifier> m_classifier; ///< Класифікатор жестів
    bool                               m_debugView{false}; ///< Стан debug-вікна
};
