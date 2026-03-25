#include "ui/SettingsWindow.h"
#include "core/ConfigManager.h"
#include "utils/Logger.h"

SettingsWindow::SettingsWindow(ConfigManager* config) : m_config(config) {}

void SettingsWindow::show() {
    m_visible = true;
    LOG_INFO("SettingsWindow shown (stub).");
    // TODO: відобразити OpenCV trackbar або нативне вікно налаштувань
}

void SettingsWindow::hide() {
    m_visible = false;
}

bool SettingsWindow::isVisible() const {
    return m_visible;
}
