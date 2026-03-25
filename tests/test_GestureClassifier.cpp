#include <gtest/gtest.h>
#include "gesture/GestureClassifier.h"
#include "gesture/HandTracker.h"

// ============================================================
// Допоміжна функція: створити HandData з вказаними даними
// ============================================================
HandData makeHand(float cx, float cy, bool detected = true) {
    HandData h;
    h.detected = detected;
    h.centerX  = cx;
    h.centerY  = cy;
    h.landmarks.push_back({cx, cy, 0.0f});
    return h;
}

// ============================================================
// Тести
// ============================================================

TEST(GestureClassifierTest, NoHandReturnsNone) {
    GestureClassifier cls(nullptr);
    HandData empty;
    empty.detected = false;

    auto result = cls.classify(empty);
    EXPECT_EQ(result.type, GestureType::NONE);
}

TEST(GestureClassifierTest, DetectedHandReturnsSomething) {
    GestureClassifier cls(nullptr);
    auto hand = makeHand(0.5f, 0.5f);

    auto result = cls.classify(hand);
    // Заглушка завжди повертає MOVE для виявленої руки
    EXPECT_NE(result.type, GestureType::NONE);
}

TEST(GestureClassifierTest, CoordinatesPassedThrough) {
    GestureClassifier cls(nullptr);
    auto hand = makeHand(0.3f, 0.7f);

    auto result = cls.classify(hand);
    EXPECT_NEAR(result.handX, 0.3f, 0.001f);
    EXPECT_NEAR(result.handY, 0.7f, 0.001f);
}

TEST(GestureClassifierTest, ConfidenceInValidRange) {
    GestureClassifier cls(nullptr);
    auto hand = makeHand(0.5f, 0.5f);

    auto result = cls.classify(hand);
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
}
