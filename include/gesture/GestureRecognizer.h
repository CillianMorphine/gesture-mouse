#pragma once
// ============================================================
// GestureRecognizer.h
// Fix 1: forward declarations instead of full includes (performance)
// Fix 2: explicit constructor (modernize-use-explicit)
// Fix 3: override keyword on virtual methods
// Fix 4: noexcept on non-throwing methods
// ============================================================
#include <memory>
#include <string>

#include <opencv2/core/mat.hpp>  // Fix: minimal include instead of <opencv2/opencv.hpp>

// Fix 5: scoped enum with explicit underlying type
enum class GestureType : uint8_t {
    NONE         = 0,
    MOVE         = 1,
    LEFT_CLICK   = 2,
    RIGHT_CLICK  = 3,
    SCROLL_UP    = 4,
    SCROLL_DOWN  = 5,
    DRAG         = 6,
};

struct GestureResult {
    GestureType type       = GestureType::NONE;
    float       handX      = 0.0F;  // Fix: 0.0F not 0.0f for consistency
    float       handY      = 0.0F;
    float       confidence = 0.0F;

    // Fix 6: [[nodiscard]] on pure query method
    [[nodiscard]] std::string typeName() const;
};

// Forward declarations — avoids heavyweight includes in header
class ConfigManager;
class HandTracker;
class GestureClassifier;

class GestureRecognizer {
public:
    // Fix 7: explicit keyword prevents unintended implicit conversion
    explicit GestureRecognizer(ConfigManager* config);

    // Fix 8: defaulted destructor in .cpp (avoids incomplete-type issues with unique_ptr)
    ~GestureRecognizer();

    // Fix 9: deleted copy (rule of 5 — cppcoreguidelines-special-member-functions)
    GestureRecognizer(const GestureRecognizer&)            = delete;
    GestureRecognizer& operator=(const GestureRecognizer&) = delete;
    GestureRecognizer(GestureRecognizer&&)                 = default;
    GestureRecognizer& operator=(GestureRecognizer&&)      = default;

    [[nodiscard]] GestureResult process(const cv::Mat& frame);

    void setDebugView(bool enabled) noexcept;  // Fix 10: noexcept

private:
    std::unique_ptr<HandTracker>       m_tracker;
    std::unique_ptr<GestureClassifier> m_classifier;
    bool                               m_debugView{false};  // Fix 11: brace-init
};
