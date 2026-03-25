#pragma once
#include "gesture/HandTracker.h"
#include "gesture/GestureRecognizer.h"

class ConfigManager;

/**
 * @brief Класифікує жест на основі даних про руку.
 *        Поточна реалізація — евристична (на основі аналізу виступів).
 */
class GestureClassifier {
public:
    explicit GestureClassifier(ConfigManager* config);

    GestureResult classify(const HandData& hand);

private:
    int  countFingers(const HandData& hand);
    bool isFist(const HandData& hand);

    // Мінімальна впевненість для підтвердження жесту
    float m_confidenceThreshold = 0.6f;
};
