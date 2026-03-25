#include "ui/TrayIcon.h"
#include "core/Application.h"
#include "utils/Logger.h"

TrayIcon::TrayIcon(Application* app) : m_app(app) {
    LOG_INFO("TrayIcon created.");
}

TrayIcon::~TrayIcon() {
    hide();
}

void TrayIcon::show() {
    // TODO: платформо-залежна реалізація
    // Windows: Shell_NotifyIcon
    // Linux: libappindicator або StatusNotifierItem
    LOG_INFO("TrayIcon shown (stub).");
}

void TrayIcon::hide() {
    LOG_INFO("TrayIcon hidden.");
}
