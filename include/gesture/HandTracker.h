#pragma once
/**
 * @file HandTracker.h
 * @brief Виявлення руки на відеокадрі методом HSV-сегментації.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * ### Алгоритм HSV-сегментації тілесного кольору
 *
 * 1. **Gaussian blur** — зменшення шуму (ядро 5×5)
 * 2. **BGR→HSV** — колірний простір HSV стійкіший до змін освітлення
 * 3. **inRange** — бінарна маска тілесного кольору
 *    - H: [0°, 20°] — жовтувато-червоний відтінок шкіри
 *    - S: [20, 255] — насиченість (виключає сірі тони)
 *    - V: [70, 255] — яскравість (виключає темряву)
 * 4. **Морфологія** — open (видаляє шум) + close (заповнює дірки)
 * 5. **findContours** — знаходить контури об'єктів на масці
 * 6. **Найбільший контур** → якщо площа > 3000 пікс² — це рука
 * 7. **Moments** — обчислення центру мас контуру
 *
 * ### Обмеження методу
 * - Чутливий до умов освітлення (рекомендується нейтральне фонове освітлення)
 * - Може давати хибні спрацьовування на об'єктах тілесного кольору
 * - Не розрізняє ліву і праву руку
 *
 * @see GestureClassifier
 */

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief Координата однієї ключової точки (landmark) руки у 3D-просторі.
 *
 * @details
 * У поточній реалізації z завжди 0 (2D-трекінг).
 * Структура сумісна з форматом MediaPipe Hand Landmarks
 * для можливого майбутнього переходу.
 *
 * Координати нормалізовані: (0,0) = лівий верхній кут кадру,
 * (1,1) = правий нижній кут.
 */
struct Landmark {
    float x{0.0F}; ///< Нормалізована X-координата [0..1]
    float y{0.0F}; ///< Нормалізована Y-координата [0..1]
    float z{0.0F}; ///< Глибина (зарезервовано, завжди 0)
};

/**
 * @brief Результат виявлення руки на одному кадрі.
 *
 * @details
 * У поточній реалізації зберігається 1 landmark (центр мас).
 * Майбутні версії планують підтримку 21 точки (MediaPipe-сумісно).
 */
struct HandData {
    bool detected{false};             ///< true якщо рука виявлена
    std::vector<Landmark> landmarks;  ///< Ключові точки (поточно: 1 — центр)
    float centerX{0.0F};              ///< Нормалізована X центру мас руки
    float centerY{0.0F};              ///< Нормалізована Y центру мас руки
};

class ConfigManager;

/**
 * @brief Детектор руки на основі HSV-сегментації тілесного кольору.
 *
 * @details
 * Клас інкапсулює весь pipeline виявлення руки.
 * GestureRecognizer отримує HandData і передає її до GestureClassifier.
 *
 * ### Налаштування кольорового діапазону
 * Параметри HSV за замовчуванням підходять для більшості умов.
 * При поганому виявленні — відрегулюйте через конфіг або SettingsWindow:
 *
 * @code{.cpp}
 * // Значення за замовчуванням (можна змінити в config)
 * m_lowerSkin = {0, 20, 70};    // H_min, S_min, V_min
 * m_upperSkin = {20, 255, 255}; // H_max, S_max, V_max
 * @endcode
 */
class HandTracker {
public:
    /**
     * @brief Конструктор. Завантажує HSV-параметри з конфігу.
     * @param config Вказівник на ConfigManager (nullable — використовує defaults).
     */
    explicit HandTracker(ConfigManager* config);

    /**
     * @brief Виявляє руку на кадрі та повертає дані про неї.
     *
     * @details
     * Повний pipeline: blur → HSV → mask → morph → contours → moments.
     * При відсутності руки або недостатній площі контуру
     * повертає `HandData{false}` без помилок.
     *
     * @param frame Вхідний кадр BGR (CV_8UC3, не порожній).
     * @return HandData з detected=true і координатами при успіху,
     *         або HandData{false} якщо рука не знайдена.
     *
     * @complexity O(W × H) де W, H — розміри кадру.
     */
    [[nodiscard]] HandData detect(const cv::Mat& frame);

    /**
     * @brief Малює landmarks і контур руки на кадрі (для debug-режиму).
     *
     * @details
     * Накладає на кадр:
     * - Зелений кружок діаметром 16px у центрі мас
     * - (майбутнє) скелет 21 landmark з MediaPipe
     *
     * @param[in,out] frame Кадр для малювання (модифікується in-place).
     * @param data HandData отримана від detect().
     *
     * @note Якщо data.detected == false — нічого не малює.
     */
    void drawLandmarks(cv::Mat& frame, const HandData& data);

private:
    /**
     * @brief Підготовка кадру: blur → HSV → бінарна маска → морфологія.
     *
     * @details
     * **Чому Gaussian blur перед HSV?**
     * Дрібний піксельний шум камери сильно фрагментує HSV-маску.
     * Blur 5×5 забирає HF-шум без розмивання меж руки.
     *
     * **Чому морфологічна обробка?**
     * - `MORPH_OPEN` (erosion + dilation): видаляє дрібні ізольовані плями
     * - `MORPH_CLOSE` (dilation + erosion): заповнює дірки всередині руки
     *
     * @param frame Вхідний BGR-кадр.
     * @return Бінарна маска (CV_8UC1) де 255 = тілесний колір.
     */
    [[nodiscard]] cv::Mat preprocessFrame(const cv::Mat& frame);

    /**
     * @brief Обчислює HandData з бінарної маски через моменти контуру.
     *
     * @details
     * Використовує `cv::moments()` для знаходження центру мас:
     * ```
     * cx = M10 / M00  (нормалізований X)
     * cy = M01 / M00  (нормалізований Y)
     * ```
     *
     * @param mask Бінарна маска (результат preprocessFrame).
     * @param original Оригінальний BGR-кадр (зарезервований для майбутніх потреб).
     * @return HandData з coordinates або HandData{false} при M00 < 1.
     */
    [[nodiscard]] HandData extractHandData(const cv::Mat& mask,
                                           const cv::Mat& original);

    cv::Scalar m_lowerSkin{0, 20, 70};    ///< Нижня межа HSV тілесного кольору
    cv::Scalar m_upperSkin{20, 255, 255}; ///< Верхня межа HSV тілесного кольору

    /// Мінімальна площа контуру для розпізнавання руки (пікселі²)
    static constexpr double kMinHandArea = 3000.0;
};
