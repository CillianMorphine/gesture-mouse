#include "core/Application.h"
#include "gesture/GestureRecognizer.h"
#include "input/MouseController.h"
#include "core/ConfigManager.h"
#include "ui/TrayIcon.h"
#include "utils/Logger.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>

Application::Application(int argc, char* argv[])
    : m_argc(argc), m_argv(argv) {
    initialize();
}

Application::~Application() {
    cleanup();
}

void Application::initialize() {
    LOG_INFO("Initializing components...");

    m_config    = std::make_unique<ConfigManager>("config/settings.json");
    m_recognizer = std::make_unique<GestureRecognizer>(m_config.get());
    m_mouseCtrl  = std::make_unique<MouseController>(m_config.get());
    m_tray       = std::make_unique<TrayIcon>(this);

    LOG_INFO("All components initialized.");
}

int Application::run() {
    m_running = true;
    LOG_INFO("Entering main loop.");

    m_tray->show();
    mainLoop();

    return EXIT_SUCCESS;
}

void Application::mainLoop() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        LOG_ERROR("Cannot open camera!");
        return;
    }

    cv::Mat frame;
    while (m_running) {
        cap >> frame;
        if (frame.empty()) continue;

        // Розпізнавання жесту
        auto gesture = m_recognizer->process(frame);

        // Застосування жесту до курсору
        m_mouseCtrl->applyGesture(gesture);

        // Обмеження частоти кадрів
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 fps
    }
}

void Application::stop() {
    LOG_INFO("Stop requested.");
    m_running = false;
}

void Application::cleanup() {
    LOG_INFO("Cleaning up resources.");
    m_tray.reset();
    m_mouseCtrl.reset();
    m_recognizer.reset();
    m_config.reset();
}
