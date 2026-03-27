#pragma once
/**
 * @file MouseController.h
 * @brief Модуль симуляції введення миші на основі розпізнаних жестів.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * MouseController перетворює абстрактні GestureResult у **конкретні
 * системні події** (переміщення, кліки, прокрутка).
 *
 * ### Платформна абстракція
 * Реалізація приховує платформо-залежний код за допомогою `#ifdef`:
 *
 * | Платформа | API симуляції | Файл |
 * |-----------|---------------|------|
 * | Windows 10/11 | `SendInput()` (user32.dll) | MouseController.cpp |
 * | Linux (X11) | `XTestFakeMotionEvent()` (xtst) | MouseController.cpp |
 *
 * ### EMA-згладжування
 * Для усунення тремтіння курсора використовується
 * **Exponential Moving Average (EMA)**:
 *
 * ```
 * smoothed_x = prev_x + α * (raw_x - prev_x)
 * ```
 * де α = `m_smoothing` ∈ (0, 1].
 * - α → 1.0: миттєве слідування (без згладжування)
 * - α → 0.0: сильне згладжування, велика інерція
 *
 * @see GestureResult
 * @see InputSimulator
 */

#include "gesture/GestureRecognizer.h"

class ConfigManager;

/**
 * @brief Розміри екрана у пікселях.
 * @details Повертається `screenSize()`. Іменована структура замість
 *          `std::pair<int,int>` — краща читабельність (readability-identifier-naming).
 */
struct ScreenSize {
    int width{0};  ///< Ширина екрана в пікселях
    int height{0}; ///< Висота екрана в пікселях
};

/**
 * @brief Контролер миші — застосовує жести як системні дії введення.
 *
 * @details
 * Клас читає налаштування з ConfigManager при ініціалізації:
 *
 * | Ключ конфігу | Тип | Замовч. | Опис |
 * |---|---|---|---|
 * | `mouse.smoothing` | float | 0.3 | Коефіцієнт EMA [0..1] |
 *
 * @par Приклад інтеграції:
 * @code{.cpp}
 * ConfigManager cfg("settings.txt");
 * MouseController ctrl(&cfg);
 *
 * GestureResult g = recognizer.process(frame);
 * ctrl.applyGesture(g);   // один виклик — вся логіка всередині
 * @endcode
 */
class MouseController {
public:
    /**
     * @brief Конструктор. Зчитує налаштування з конфігу.
     * @param config Вказівник на ConfigManager (не nullptr, не власник).
     */
    explicit MouseController(ConfigManager* config);

    /** @brief Деструктор. Відпускає кнопку миші якщо drag активний. */
    ~MouseController();

    /// @name Rule of 5
    /// @{
    MouseController(const MouseController&)            = delete;
    MouseController& operator=(const MouseController&) = delete;
    MouseController(MouseController&&)                 = default;
    MouseController& operator=(MouseController&&)      = default;
    /// @}

    /**
     * @brief Застосовує розпізнаний жест як системну дію миші.
     *
     * @details
     * Відповідність GestureType → дія:
     *
     * | GestureType | Виклик | Опис |
     * |---|---|---|
     * | MOVE | `moveCursor(x, y)` | EMA-плавне переміщення |
     * | LEFT_CLICK | `leftClick()` | MouseDown + MouseUp |
     * | RIGHT_CLICK | `rightClick()` | MouseDown + MouseUp |
     * | SCROLL_UP | `scroll(+3)` | 3 кроки прокрутки вгору |
     * | SCROLL_DOWN | `scroll(-3)` | 3 кроки прокрутки вниз |
     * | DRAG | *(TODO)* | MouseDown без MouseUp |
     * | NONE | *(нічого)* | — |
     *
     * @param gesture Результат розпізнавання з GestureRecognizer::process().
     *
     * @note Клік відбувається миттєво (down+up в одному виклику).
     *       Для утримання кнопки (drag) стан зберігається в m_dragActive.
     */
    void applyGesture(const GestureResult& gesture);

    /**
     * @brief Повертає поточні розміри основного екрана.
     *
     * @details
     * Реалізація:
     * - **Windows:** `GetSystemMetrics(SM_CXSCREEN / SM_CYSCREEN)`
     * - **Linux:** `DefaultScreenOfDisplay(XOpenDisplay(nullptr))`
     * - **Інше:** hardcoded 1920×1080 як fallback
     *
     * @return ScreenSize з шириною та висотою в пікселях.
     *
     * @note noexcept — у разі помилки повертає {1920, 1080}.
     */
    [[nodiscard]] static ScreenSize screenSize() noexcept;

private:
    /**
     * @brief Переміщує курсор із EMA-згладжуванням.
     *
     * @details
     * **Алгоритм:**
     * 1. Застосовує EMA: `m_prevX += m_smoothing * (normX - m_prevX)`
     * 2. Денормалізує: `px = m_prevX * screenWidth`
     * 3. Викликає платформний API
     *
     * @param normX Нормалізована X-координата з GestureResult::handX [0..1].
     * @param normY Нормалізована Y-координата з GestureResult::handY [0..1].
     *
     * @pre 0.0F <= normX <= 1.0F && 0.0F <= normY <= 1.0F
     */
    void moveCursor(float normX, float normY);

    /**
     * @brief Симулює клік лівою кнопкою миші (down + up).
     * @details Windows: MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP via SendInput.
     *          Linux: XTestFakeButtonEvent(dpy, 1, True/False).
     */
    void leftClick();

    /**
     * @brief Симулює клік правою кнопкою миші (down + up).
     */
    void rightClick();

    /**
     * @brief Симулює прокрутку коліщатком миші.
     *
     * @param delta Кількість кроків. Позитивне — вгору, від'ємне — вниз.
     *              Один крок = WHEEL_DELTA (120) на Windows, кнопка 4/5 на Linux.
     */
    void scroll(int delta);

    float m_smoothing{0.3F};   ///< Коефіцієнт EMA-фільтру [0..1]
    float m_prevX{0.0F};       ///< Попередня X-координата (після згладжування)
    float m_prevY{0.0F};       ///< Попередня Y-координата (після згладжування)
    bool  m_dragActive{false}; ///< Прапорець активного режиму перетягування
};
