#include "gesture/HandTracker.h"
#include "core/ConfigManager.h"
#include "utils/Logger.h"

HandTracker::HandTracker(ConfigManager* config) {
    // Завантаження параметрів тілесного кольору з конфігу (якщо є)
    LOG_INFO("HandTracker initialized (skin-color method).");
}

cv::Mat HandTracker::preprocessFrame(const cv::Mat& frame) {
    cv::Mat blurred, hsv, mask;
    cv::GaussianBlur(frame, blurred, {5, 5}, 0);
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, m_lowerSkin, m_upperSkin, mask);

    // Морфологічна обробка для усунення шуму
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5});
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN,  kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
    return mask;
}

HandData HandTracker::detect(const cv::Mat& frame) {
    cv::Mat mask = preprocessFrame(frame);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return HandData{false};

    // Найбільший контур — це рука
    auto maxIt = std::max_element(contours.begin(), contours.end(),
        [](const auto& a, const auto& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });

    double area = cv::contourArea(*maxIt);
    if (area < 3000.0) return HandData{false}; // занадто малий об'єкт

    return extractHandData(mask, frame);
}

HandData HandTracker::extractHandData(const cv::Mat& mask, const cv::Mat& /*original*/) {
    cv::Moments m = cv::moments(mask, true);
    if (m.m00 < 1.0) return HandData{false};

    HandData data;
    data.detected = true;
    data.centerX  = static_cast<float>(m.m10 / m.m00) / mask.cols;
    data.centerY  = static_cast<float>(m.m01 / m.m00) / mask.rows;

    // Спрощена модель: один "ключовий" landmark у центрі маси
    data.landmarks.push_back({data.centerX, data.centerY, 0.0f});
    return data;
}

void HandTracker::drawLandmarks(cv::Mat& frame, const HandData& data) {
    if (!data.detected) return;
    int cx = static_cast<int>(data.centerX * frame.cols);
    int cy = static_cast<int>(data.centerY * frame.rows);
    cv::circle(frame, {cx, cy}, 8, {0, 255, 0}, -1);
}
