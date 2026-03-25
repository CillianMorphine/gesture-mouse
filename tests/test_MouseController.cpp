#include <gtest/gtest.h>
#include "input/MouseController.h"

TEST(MouseControllerTest, ScreenSizeIsPositive) {
    auto [w, h] = MouseController::screenSize();
    // Навіть у headless-середовищі розміри не повинні бути від'ємними
    EXPECT_GE(w, 0);
    EXPECT_GE(h, 0);
}

TEST(MouseControllerTest, ApplyNoneGestureDoesNotCrash) {
    ConfigManager cfg("/tmp/nonexistent.txt");
    MouseController ctrl(&cfg);

    GestureResult g;
    g.type = GestureType::NONE;
    EXPECT_NO_THROW(ctrl.applyGesture(g));
}
