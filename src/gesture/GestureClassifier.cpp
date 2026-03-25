#include "gesture/GestureClassifier.h"
#include "core/ConfigManager.h"

GestureClassifier::GestureClassifier(ConfigManager* /*config*/) {}

GestureResult GestureClassifier::classify(const HandData& hand) {
    if (!hand.detected) return GestureResult{GestureType::NONE};

    GestureResult result;
    result.handX      = hand.centerX;
    result.handY      = hand.centerY;
    result.confidence = 0.85f; // Спрощено; у повній версії — результат нейромережі

    int fingers = countFingers(hand);

    if (fingers >= 4) {
        result.type = GestureType::MOVE;
    } else if (fingers == 1) {
        result.type = GestureType::LEFT_CLICK;
    } else if (fingers == 2) {
        result.type = GestureType::RIGHT_CLICK;
    } else if (isFist(hand)) {
        result.type = GestureType::DRAG;
    } else {
        result.type = GestureType::NONE;
    }

    return result;
}

int GestureClassifier::countFingers(const HandData& hand) {
    // Спрощена евристика: у повній реалізації — аналіз опуклої оболонки
    (void)hand;
    return 4; // Заглушка
}

bool GestureClassifier::isFist(const HandData& hand) {
    (void)hand;
    return false; // Заглушка
}
