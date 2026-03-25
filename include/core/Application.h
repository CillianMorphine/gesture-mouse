#pragma once
#include <memory>
#include <atomic>

class GestureRecognizer;
class MouseController;
class ConfigManager;
class TrayIcon;

/**
 * @brief Головний клас програми. Керує життєвим циклом усіх компонентів.
 */
class Application {
public:
    Application(int argc, char* argv[]);
    ~Application();

    /// Запуск головного циклу програми
    int run();

    /// Запит на зупинку програми
    void stop();

private:
    void initialize();
    void mainLoop();
    void cleanup();

    std::unique_ptr<GestureRecognizer> m_recognizer;
    std::unique_ptr<MouseController>   m_mouseCtrl;
    std::unique_ptr<ConfigManager>     m_config;
    std::unique_ptr<TrayIcon>          m_tray;

    std::atomic<bool> m_running{false};
    int m_argc;
    char** m_argv;
};
