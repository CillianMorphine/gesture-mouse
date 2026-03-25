#include "gesture/GestureRecognizer.h"
#include "gesture/HandTracker.h"
#include "gesture/GestureClassifier.h"
#include "core/ConfigManager.h"
#include "utils/Logger.h"

std::string GestureResult::typeName() const {
    switch (type) {
        case GestureType::NONE:        return "NONE";
        case GestureType::MOVE:        return "MOVE";
        case GestureType::LEFT_CLICK:  return "LEFT_CLICK";
        case GestureType::RIGHT_CLICK: return "RIGHT_CLICK";
        case GestureType::SCROLL_UP:   return "SCROLL_UP";
        case GestureType::SCROLL_DOWN: return "SCROLL_DOWN";
        case GestureType::DRAG:        return "DRAG";
        default:                       return "UNKNOWN";
    }
}

GestureRecognizer::GestureRecognizer(ConfigManager* config) {
    m_tracker    = std::make_unique<HandTracker>(config);
    m_classifier = std::make_unique<GestureClassifier>(config);
    LOG_INFO("GestureRecognizer created.");
}

GestureRecognizer::~GestureRecognizer() = default;

GestureResult GestureRecognizer::process(const cv::Mat& frame) {
    // 1. Виявити руку на кадрі
    auto handData = m_tracker->detect(frame);
    if (!handData.detected) {
        return GestureResult{GestureType::NONE};
    }

    // 2. Класифікувати жест на основі ключових точок
    GestureResult result = m_classifier->classify(handData);

    // 3. Відображення для налагодження
    if (m_debugView) {
        cv::Mat dbg = frame.clone();
        m_tracker->drawLandmarks(dbg, handData);
        cv::putText(dbg, result.typeName(), {10, 30},
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,255,0}, 2);
        cv::imshow("GestureMouse Debug", dbg);
        cv::waitKey(1);
    }

    return result;
}

void GestureRecognizer::setDebugView(bool enabled) {
    m_debugView = enabled;
    if (!enabled) cv::destroyWindow("GestureMouse Debug");
}
