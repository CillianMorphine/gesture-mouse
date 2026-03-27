#pragma once
// ============================================================
// MouseController.h
// Fix 1: [[nodiscard]] on screenSize()
// Fix 2: noexcept on non-throwing methods
// Fix 3: brace-initialization for member variables
// Fix 4: std::pair → struct for named return (readability)
// ============================================================
#include <utility>

#include "gesture/GestureRecognizer.h"

class ConfigManager;

// Fix 4: named struct instead of std::pair (readability-identifier-naming)
struct ScreenSize {
    int width  = 0;
    int height = 0;
};

class MouseController {
public:
    explicit MouseController(ConfigManager* config);
    ~MouseController();

    MouseController(const MouseController&)            = delete;
    MouseController& operator=(const MouseController&) = delete;
    MouseController(MouseController&&)                 = default;
    MouseController& operator=(MouseController&&)      = default;

    void applyGesture(const GestureResult& gesture);

    // Fix 1: [[nodiscard]] — caller should use the return value
    [[nodiscard]] static ScreenSize screenSize() noexcept;

private:
    void moveCursor(float normX, float normY);
    void leftClick();
    void rightClick();
    void scroll(int delta);

    // Fix 3: brace-init instead of = init (modernize-use-default-member-init)
    float m_smoothing{0.3F};
    float m_prevX{0.0F};
    float m_prevY{0.0F};
    bool  m_dragActive{false};
};
