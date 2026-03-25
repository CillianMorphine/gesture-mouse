#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

/// Координата ключової точки руки
struct Landmark {
    float x, y, z;
};

/// Дані про виявлену руку
struct HandData {
    bool detected = false;
    std::vector<Landmark> landmarks; ///< 21 ключова точка (MediaPipe-сумісна)
    float centerX = 0.0f;
    float centerY = 0.0f;
};

class ConfigManager;

/**
 * @brief Виявляє руку на кадрі та повертає координати ключових точок.
 *        Використовує метод тілесного кольору з фільтрацією контурів.
 */
class HandTracker {
public:
    explicit HandTracker(ConfigManager* config);

    HandData detect(const cv::Mat& frame);
    void drawLandmarks(cv::Mat& frame, const HandData& data);

private:
    cv::Mat preprocessFrame(const cv::Mat& frame);
    HandData extractHandData(const cv::Mat& mask, const cv::Mat& original);

    // HSV-діапазон тілесного кольору (налаштовується через конфіг)
    cv::Scalar m_lowerSkin{0, 20, 70};
    cv::Scalar m_upperSkin{20, 255, 255};
};
