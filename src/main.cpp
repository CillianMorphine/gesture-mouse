#include "core/Application.h"
#include "utils/Logger.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        Logger::init("gesture_mouse.log");
        LOG_INFO("GestureMouse starting...");

        Application app(argc, argv);
        return app.run();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
