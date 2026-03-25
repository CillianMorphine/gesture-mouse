#pragma once

class Application;

/**
 * @brief Іконка у системному треї.
 *        Надає користувачу доступ до налаштувань та виходу.
 */
class TrayIcon {
public:
    explicit TrayIcon(Application* app);
    ~TrayIcon();

    void show();
    void hide();

private:
    Application* m_app;
};
