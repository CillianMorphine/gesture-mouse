#include "input/InputSimulator.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif defined(__linux__)
  #include <X11/Xlib.h>
  #include <X11/extensions/XTest.h>
#endif

void InputSimulator::moveMouse(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (d) { XTestFakeMotionEvent(d, -1, x, y, CurrentTime); XFlush(d); XCloseDisplay(d); }
#endif
}

void InputSimulator::mouseButtonDown(int button) {
#ifdef _WIN32
    INPUT in{}; in.type = INPUT_MOUSE;
    in.mi.dwFlags = (button == 1) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &in, sizeof(INPUT));
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (d) { XTestFakeButtonEvent(d, button, True, CurrentTime); XFlush(d); XCloseDisplay(d); }
#endif
}

void InputSimulator::mouseButtonUp(int button) {
#ifdef _WIN32
    INPUT in{}; in.type = INPUT_MOUSE;
    in.mi.dwFlags = (button == 1) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
    SendInput(1, &in, sizeof(INPUT));
#elif defined(__linux__)
    Display* d = XOpenDisplay(nullptr);
    if (d) { XTestFakeButtonEvent(d, button, False, CurrentTime); XFlush(d); XCloseDisplay(d); }
#endif
}

void InputSimulator::mouseScroll(int delta) {
#ifdef _WIN32
    INPUT in{}; in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_WHEEL;
    in.mi.mouseData = static_cast<DWORD>(delta * 120);
    SendInput(1, &in, sizeof(INPUT));
#elif defined(__linux__)
    int btn = (delta > 0) ? 4 : 5;
    Display* d = XOpenDisplay(nullptr);
    if (d) {
        for (int i = 0; i < (delta < 0 ? -delta : delta); ++i) {
            XTestFakeButtonEvent(d, btn, True,  CurrentTime);
            XTestFakeButtonEvent(d, btn, False, CurrentTime);
        }
        XFlush(d); XCloseDisplay(d);
    }
#endif
}

void InputSimulator::keyDown(int keyCode) {
#ifdef _WIN32
    INPUT in{}; in.type = INPUT_KEYBOARD;
    in.ki.wVk = static_cast<WORD>(keyCode);
    SendInput(1, &in, sizeof(INPUT));
#endif
}

void InputSimulator::keyUp(int keyCode) {
#ifdef _WIN32
    INPUT in{}; in.type = INPUT_KEYBOARD;
    in.ki.wVk = static_cast<WORD>(keyCode);
    in.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
#endif
}
