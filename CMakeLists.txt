cmake_minimum_required(VERSION 3.15)
project(gesture_mouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Опція профілювання ───────────────────────────────────────────────────────
# Вмикає вбудований профілювальник (src/utils/profiler.hpp).
# Вимкніть у production-збірці для нульових накладних витрат.
#
# Використання: cmake -DGM_PROFILING_ENABLED=ON ..
option(GM_PROFILING_ENABLED "Enable built-in performance profiling" OFF)

# ─── Профіль збірки ───────────────────────────────────────────────────────────
# Release:              cmake -DCMAKE_BUILD_TYPE=Release ..
# Debug:                cmake -DCMAKE_BUILD_TYPE=Debug ..
# RelWithDebInfo:       cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
# Профілювання (gprof): додати -DCMAKE_CXX_FLAGS="-pg"

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ─── Залежності ───────────────────────────────────────────────────────────────
find_package(OpenCV 4 REQUIRED)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# ─── Основний виконуваний файл ────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/utils/logger.cpp
    src/utils/errors.cpp
    src/utils/profiler.cpp
    src/gesture/gesture_detector.cpp
    src/gesture/gesture_detector_optimized.cpp
)

add_executable(gesture_mouse ${SOURCES})

target_include_directories(gesture_mouse PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(gesture_mouse PRIVATE
    ${OpenCV_LIBS}
    spdlog::spdlog
)

if(GM_PROFILING_ENABLED)
    target_compile_definitions(gesture_mouse PRIVATE GM_PROFILING_ENABLED)
    message(STATUS "  [profiling] Вбудований профілювальник: УВІМКНЕНО")
else()
    message(STATUS "  [profiling] Вбудований профілювальник: вимкнено")
endif()

# ─── Прапори оптимізації ─────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(gesture_mouse PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native -DNDEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>
    )
endif()

# ─── Бенчмарк (окремий виконуваний файл) ─────────────────────────────────────
add_executable(gesture_mouse_bench
    tests/benchmark.cpp
)
target_include_directories(gesture_mouse_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(gesture_mouse_bench PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O2>
    $<$<CXX_COMPILER_ID:MSVC>:/O2>
)

# ─── Порівняльний бенчмарк ────────────────────────────────────────────────────
add_executable(gesture_mouse_bench_compare
    tests/benchmark_compare.cpp
)
target_include_directories(gesture_mouse_bench_compare PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(gesture_mouse_bench_compare PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O2>
)

# ─── Платформо-залежні бібліотеки ────────────────────────────────────────────
if(WIN32)
    target_link_libraries(gesture_mouse PRIVATE user32)
    target_compile_definitions(gesture_mouse PRIVATE GM_PLATFORM_WINDOWS)
elseif(UNIX)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(gesture_mouse PRIVATE ${X11_LIBRARIES})
    endif()
    target_compile_definitions(gesture_mouse PRIVATE GM_PLATFORM_LINUX)
endif()

# ─── Копіювання конфігу ───────────────────────────────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/gesture_mouse.conf
    ${CMAKE_BINARY_DIR}/gesture_mouse.conf
    COPYONLY
)

message(STATUS "GestureMouse ${PROJECT_VERSION} — конфігурацію завершено")
message(STATUS "  OpenCV: ${OpenCV_VERSION}")
message(STATUS "  C++ std: ${CMAKE_CXX_STANDARD}")
