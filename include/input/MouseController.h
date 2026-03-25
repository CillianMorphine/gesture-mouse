#pragma once
#include "gesture/GestureRecognizer.h"

class ConfigManager;

/**
 * @brief Перетворює жест на системні дії миші.
 *        Абстрагує платформо-залежний код (Win32 / X11).
 */
class MouseController {
public:
    explicit MouseController(ConfigManager* config);
    ~MouseController();

    /// Застосувати розпізнаний жест
    void applyGesture(const GestureResult& gesture);

    /// Отримати поточні розміри екрана
    static std::pair<int,int> screenSize();

private:
    void moveCursor(float normX, float normY);
    void leftClick();
    void rightClick();
    void scroll(int delta);

    // Коефіцієнт згладжування переміщення (0..1)
    float m_smoothing = 0.3f;

    // Попередня позиція для згладжування
    float m_prevX = 0.0f;
    float m_prevY = 0.0f;

    // Стан кнопки (чи натиснуто для drag)
    bool m_dragActive = false;
};
