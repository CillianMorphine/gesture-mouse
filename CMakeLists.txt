cmake_minimum_required(VERSION 3.15)
project(gesture_mouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Залежності: OpenCV ───────────────────────────────────────────────────────
find_package(OpenCV 4 REQUIRED)

# ─── Залежності: spdlog (через FetchContent) ─────────────────────────────────
# spdlog — заголовочна (header-only) бібліотека для логування.
# CMake автоматично завантажить її при першій конфігурації.
# Документація: https://github.com/gabime/spdlog
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1   # стабільна версія
)
FetchContent_MakeAvailable(spdlog)

# ─── Вихідні файли ───────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp

    # Утиліти (логування та обробка помилок)
    src/utils/logger.cpp
    src/utils/errors.cpp

    # Ядро (розкоментувати при реалізації)
    # src/core/application.cpp

    # Розпізнавання жестів
    src/gesture/gesture_detector.cpp

    # Симуляція введення
    # src/input/mouse_controller.cpp

    # Інтерфейс (трей)
    # src/ui/tray_icon.cpp
)

# ─── Виконуваний файл ─────────────────────────────────────────────────────────
add_executable(gesture_mouse ${SOURCES})

target_include_directories(gesture_mouse PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(gesture_mouse PRIVATE
    ${OpenCV_LIBS}
    spdlog::spdlog
)

# ─── Платформо-залежні бібліотеки ────────────────────────────────────────────
if(WIN32)
    # Win32 API для SendInput (симуляція миші)
    target_link_libraries(gesture_mouse PRIVATE user32)
elseif(UNIX)
    # X11 / XTest для симуляції введення на Linux
    find_package(X11 REQUIRED)
    target_link_libraries(gesture_mouse PRIVATE ${X11_LIBRARIES} Xtst)
endif()

# ─── Визначення для коду ─────────────────────────────────────────────────────
if(WIN32)
    target_compile_definitions(gesture_mouse PRIVATE GM_PLATFORM_WINDOWS)
else()
    target_compile_definitions(gesture_mouse PRIVATE GM_PLATFORM_LINUX)
endif()

# ─── Копіювання конфіг-файлу в директорію збірки ─────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/gesture_mouse.conf
    ${CMAKE_BINARY_DIR}/gesture_mouse.conf
    COPYONLY
)

# ─── Тести (Google Test) ─────────────────────────────────────────────────────
# enable_testing()
# add_subdirectory(tests)

message(STATUS "GestureMouse ${PROJECT_VERSION} — конфігурацію завершено")
message(STATUS "  Компілятор:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  OpenCV:       ${OpenCV_VERSION}")
message(STATUS "  Стандарт C++: ${CMAKE_CXX_STANDARD}")
