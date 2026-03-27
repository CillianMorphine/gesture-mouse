/**
 * @file gesture_detector.cpp
 * @brief Приклад інтеграції логування у модулі розпізнавання жестів.
 *
 * Демонструє використання GM_LOG_* макросів та обробки помилок
 * у реальному коді застосунку.
 */

#include "../utils/logger.hpp"
#include "../utils/errors.hpp"
// #include <opencv2/opencv.hpp>

#include <string>

namespace gm {

// Фіктивні типи для демонстрації (замінити реальними)
enum class GestureType { NONE, MOVE, LEFT_CLICK, RIGHT_CLICK, SCROLL_UP, SCROLL_DOWN, DRAG };

static std::string gestureToString(GestureType g) {
    switch (g) {
        case GestureType::MOVE:        return "MOVE";
        case GestureType::LEFT_CLICK:  return "LEFT_CLICK";
        case GestureType::RIGHT_CLICK: return "RIGHT_CLICK";
        case GestureType::SCROLL_UP:   return "SCROLL_UP";
        case GestureType::SCROLL_DOWN: return "SCROLL_DOWN";
        case GestureType::DRAG:        return "DRAG";
        default:                       return "NONE";
    }
}

/**
 * @brief Детектор жестів із інтегрованим логуванням.
 */
class GestureDetector {
public:
    explicit GestureDetector(int camera_index = 0)
        : camera_index_(camera_index)
    {
        GM_INFO("[gesture] Ініціалізація детектора жестів (camera_index={})", camera_index_);
    }

    /**
     * @brief Ініціалізує підключення до камери.
     * @throws CameraError якщо камеру не вдалося відкрити.
     */
    void initialize() {
        GM_DEBUG("[gesture] Спроба відкрити камеру index={}", camera_index_);

        // Симуляція помилки для демонстрації (реально: cap_.open(camera_index_))
        bool camera_ok = true; // замінити: cap_.open(camera_index_)

        if (!camera_ok) {
            GM_THROW(CameraError,
                     ErrorCode::CameraNotFound,
                     "Не вдалося відкрити камеру",
                     "camera_index=" + std::to_string(camera_index_) +
                     ", os=" +
#if defined(_WIN32)
                     "Windows"
#else
                     "Linux"
#endif
            );
        }

        GM_INFO("[gesture] Камеру index={} відкрито успішно", camera_index_);
    }

    /**
     * @brief Зчитує кадр і класифікує жест.
     * @param[out] confidence Впевненість класифікатора (0.0–1.0).
     * @return Розпізнаний тип жесту.
     */
    GestureType detect(float& confidence) {
        GM_TRACE("[gesture] detect() — зчитування кадру");

        // Симуляція (реально: cap_.read(frame_))
        bool frame_ok = true;
        if (!frame_ok) {
            GM_THROW(CameraError,
                     ErrorCode::CameraReadFail,
                     "Збій зчитування кадру",
                     "frame_count=" + std::to_string(frame_count_));
        }
        ++frame_count_;

        // Симуляція класифікації жесту
        confidence = 0.87f;
        GestureType result = GestureType::MOVE;

        if (confidence < kMinConfidence_) {
            GM_WARN("[gesture] Низька впевненість {:.2f} < {:.2f} для жесту {}",
                    confidence, kMinConfidence_, gestureToString(result));
            return GestureType::NONE;
        }

        GM_DEBUG("[gesture] Жест розпізнано: {} (confidence={:.2f})",
                 gestureToString(result), confidence);
        return result;
    }

    ~GestureDetector() {
        GM_INFO("[gesture] Детектор завершено. Оброблено кадрів: {}", frame_count_);
    }

private:
    int         camera_index_;
    int         frame_count_    = 0;
    float       kMinConfidence_ = 0.6f;
};

} // namespace gm
