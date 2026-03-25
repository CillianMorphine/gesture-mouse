#pragma once

class ConfigManager;

/**
 * @brief Вікно налаштувань (чутливість, кольоровий діапазон, камера).
 */
class SettingsWindow {
public:
    explicit SettingsWindow(ConfigManager* config);

    void show();
    void hide();
    bool isVisible() const;

private:
    ConfigManager* m_config;
    bool m_visible = false;
};
