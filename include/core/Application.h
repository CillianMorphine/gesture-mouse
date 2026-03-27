#pragma once
// ============================================================
// Application.h
// Fix 1: <atomic> included in header (was missing)
// Fix 2: Rule of 5 — deleted copy/move
// Fix 3: [[nodiscard]] on run()
// Fix 4: noexcept on stop()
// ============================================================
#include <atomic>
#include <memory>

class GestureRecognizer;
class MouseController;
class ConfigManager;
class TrayIcon;

class Application {
public:
    Application(int argc, char** argv);  // Fix 5: char** not char*[]
    ~Application();

    // Fix 2: rule of 5
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    // Fix 3: caller must check return code
    [[nodiscard]] int run();

    // Fix 4: stop() cannot throw
    void stop() noexcept;

private:
    void initialize();
    void mainLoop();
    void cleanup();

    std::unique_ptr<GestureRecognizer> m_recognizer;
    std::unique_ptr<MouseController>   m_mouseCtrl;
    std::unique_ptr<ConfigManager>     m_config;
    std::unique_ptr<TrayIcon>          m_tray;

    std::atomic<bool> m_running{false};

    // Fix 6: store argc/argv as members safely
    int    m_argc{0};
    char** m_argv{nullptr};
};
