cmake_minimum_required(VERSION 3.15)

# Знаходимо або завантажуємо Google Test
find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(gesture_mouse_tests
        test_GestureClassifier.cpp
        test_MouseController.cpp
        test_ConfigManager.cpp
    )

    target_include_directories(gesture_mouse_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(gesture_mouse_tests
        GTest::GTest
        GTest::Main
        ${OpenCV_LIBS}
    )

    add_test(NAME GestureMouseTests COMMAND gesture_mouse_tests)
else()
    message(WARNING "GTest not found — skipping tests.")
endif()
