#pragma once
#include <opencv2/opencv.hpp>
#include <string>

/// Перелік можливих жестів
enum class GestureType {
    NONE,           ///< Рука не виявлена або невизначений жест
    MOVE,           ///< Переміщення курсору (відкрита долоня)
    LEFT_CLICK,     ///< Клік лівою кнопкою (вказівний палець)
    RIGHT_CLICK,    ///< Клік правою кнопкою (два пальці)
    SCROLL_UP,      ///< Прокрутка вгору
    SCROLL_DOWN,    ///< Прокрутка вниз
    DRAG,           ///< Перетягування (стиснутий кулак у русі)
};

/// Результат розпізнавання одного кадру
struct GestureResult {
    GestureType type = GestureType::NONE;
    float       handX = 0.0f;   ///< Нормалізована координата X [0..1]
    float       handY = 0.0f;   ///< Нормалізована координата Y [0..1]
    float       confidence = 0.0f; ///< Впевненість класифікатора [0..1]

    std::string typeName() const;
};

class ConfigManager;
class HandTracker;
class GestureClassifier;

/**
 * @brief Фасад модуля розпізнавання: об'єднує трекінг руки та класифікацію жесту.
 */
class GestureRecognizer {
public:
    explicit GestureRecognizer(ConfigManager* config);
    ~GestureRecognizer();

    /// Обробити один кадр і повернути результат
    GestureResult process(const cv::Mat& frame);

    /// Увімкнути/вимкнути відображення debug-вікна
    void setDebugView(bool enabled);

private:
    std::unique_ptr<HandTracker>      m_tracker;
    std::unique_ptr<GestureClassifier> m_classifier;
    bool m_debugView = false;
};
