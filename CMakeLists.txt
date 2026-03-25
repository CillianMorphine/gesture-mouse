cmake_minimum_required(VERSION 3.15)
project(GestureMouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===========================
# Find dependencies
# ===========================
find_package(OpenCV REQUIRED)

# ===========================
# Include directories
# ===========================
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# ===========================
# Source files
# ===========================
set(CORE_SOURCES
    src/core/Application.cpp
    src/core/ConfigManager.cpp
)

set(GESTURE_SOURCES
    src/gesture/GestureRecognizer.cpp
    src/gesture/HandTracker.cpp
    src/gesture/GestureClassifier.cpp
)

set(INPUT_SOURCES
    src/input/MouseController.cpp
    src/input/InputSimulator.cpp
)

set(UI_SOURCES
    src/ui/TrayIcon.cpp
    src/ui/SettingsWindow.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Timer.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${GESTURE_SOURCES}
    ${INPUT_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
)

# ===========================
# Executable
# ===========================
add_executable(gesture_mouse ${ALL_SOURCES})

target_link_libraries(gesture_mouse
    ${OpenCV_LIBS}
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(gesture_mouse user32 gdi32)
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(gesture_mouse ${X11_LIBRARIES})
endif()

# ===========================
# Tests
# ===========================
enable_testing()
add_subdirectory(tests)

# ===========================
# Compiler warnings
# ===========================
if(MSVC)
    target_compile_options(gesture_mouse PRIVATE /W4)
else()
    target_compile_options(gesture_mouse PRIVATE -Wall -Wextra -Wpedantic)
endif()
