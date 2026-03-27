cmake_minimum_required(VERSION 3.15)
project(GestureMouse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Export compile commands for clang-tidy ───────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Find dependencies ────────────────────────────────────────
find_package(OpenCV REQUIRED)

# ── Include dirs ─────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include ${OpenCV_INCLUDE_DIRS})

# ── Sources ──────────────────────────────────────────────────
file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(gesture_mouse ${ALL_SOURCES})
target_link_libraries(gesture_mouse ${OpenCV_LIBS})

if(WIN32)
    target_link_libraries(gesture_mouse user32 gdi32)
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(gesture_mouse ${X11_LIBRARIES})
endif()

# ── Strict compiler warnings ─────────────────────────────────
if(MSVC)
    target_compile_options(gesture_mouse PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(gesture_mouse PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Wunused -Woverloaded-virtual
        -Wconversion -Wsign-conversion -Wnull-dereference
        -Wdouble-promotion -Wformat=2
    )
endif()

# ════════════════════════════════════════════════════════════
# LINT TARGETS (Lab 4)
# ════════════════════════════════════════════════════════════

# ── clang-tidy target ────────────────────────────────────────
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    # Attach to build (optional — enable in CI)
    option(ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)
    if(ENABLE_CLANG_TIDY)
        set_target_properties(gesture_mouse PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY};--warnings-as-errors=*")
    endif()

    # Standalone target: cmake --build build --target lint
    add_custom_target(lint
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lint.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy + cppcheck..."
        VERBATIM
    )

    add_custom_target(lint-fix
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lint.sh --fix --format
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running linters with auto-fix..."
        VERBATIM
    )
endif()

# ── clang-format target ──────────────────────────────────────
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i
            $<TARGET_PROPERTY:gesture_mouse,SOURCES>
        COMMENT "Formatting source files with clang-format..."
        VERBATIM
    )

    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror
            $<TARGET_PROPERTY:gesture_mouse,SOURCES>
        COMMENT "Checking code formatting..."
        VERBATIM
    )
endif()

# ── Tests ────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
