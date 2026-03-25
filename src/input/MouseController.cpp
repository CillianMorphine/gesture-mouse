#include "input/MouseController.h"
#include "core/ConfigManager.h"
#include "utils/Logger.h"
#include <cmath>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif defined(__linux__)
  #include <X11/Xlib.h>
  #include <X11/extensions/XTest.h>
#endif

MouseController::MouseController(ConfigManager* config) {
    LOG_INFO("MouseController initialized.");
    // m_smoothing = config->get<float>("mouse.smoothing", 0.3f);
}

MouseController::~MouseController() = default;

void MouseController::applyGesture(const GestureResult& gesture) {
    switch (gesture.type) {
        case GestureType::MOVE:
            moveCursor(gesture.handX, gesture.handY);
            break;
        case GestureType::LEFT_CLICK:
            leftClick();
            break;
        case GestureType::RIGHT_CLICK:
            rightClick();
            break;
        case GestureType::SCROLL_UP:
            scroll(3);
            break;
        case GestureType::SCROLL_DOWN:
            scroll(-3);
            break;
        case GestureType::DRAG:
            // TODO: реалізувати перетягування
            break;
        default:
            break;
    }
}

void MouseController::moveCursor(float normX, float normY) {
    // Згладжування (exponential moving average)
    m_prevX = m_prevX + m_smoothing * (normX - m_prevX);
    m_prevY = m_prevY + m_smoothing * (normY - m_prevY);

    auto [sw, sh] = screenSize();
    int px = static_cast<int>(m_prevX * sw);
    int py = static_cast<int>(m_prevY * sh);

#ifdef _WIN32
    SetCursorPos(px, py);
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy) {
        XTestFakeMotionEvent(dpy, -1, px, py, CurrentTime);
        XFlush(dpy);
        XCloseDisplay(dpy);
    }
#endif
}

void MouseController::leftClick() {
#ifdef _WIN32
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE; input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE; input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy) {
        XTestFakeButtonEvent(dpy, 1, True,  CurrentTime);
        XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
        XFlush(dpy);
        XCloseDisplay(dpy);
    }
#endif
}

void MouseController::rightClick() {
#ifdef _WIN32
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE; input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    input[1].type = INPUT_MOUSE; input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(2, input, sizeof(INPUT));
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy) {
        XTestFakeButtonEvent(dpy, 3, True,  CurrentTime);
        XTestFakeButtonEvent(dpy, 3, False, CurrentTime);
        XFlush(dpy);
        XCloseDisplay(dpy);
    }
#endif
}

void MouseController::scroll(int delta) {
#ifdef _WIN32
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(delta * WHEEL_DELTA);
    SendInput(1, &input, sizeof(INPUT));
#elif defined(__linux__)
    int btn = (delta > 0) ? 4 : 5;
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy) {
        for (int i = 0; i < std::abs(delta); ++i) {
            XTestFakeButtonEvent(dpy, btn, True,  CurrentTime);
            XTestFakeButtonEvent(dpy, btn, False, CurrentTime);
        }
        XFlush(dpy);
        XCloseDisplay(dpy);
    }
#endif
}

std::pair<int,int> MouseController::screenSize() {
#ifdef _WIN32
    return {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return {1920, 1080};
    Screen* scr = DefaultScreenOfDisplay(dpy);
    int w = scr->width, h = scr->height;
    XCloseDisplay(dpy);
    return {w, h};
#else
    return {1920, 1080};
#endif
}
