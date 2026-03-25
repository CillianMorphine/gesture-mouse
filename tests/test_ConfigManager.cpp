#include <gtest/gtest.h>
#include "core/ConfigManager.h"
#include <fstream>
#include <cstdio>

// Шлях до тимчасового конфіг-файлу для тестів
static const std::string TEST_CONFIG = "/tmp/test_gesture_config.txt";

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Записати тестовий конфіг
        std::ofstream f(TEST_CONFIG);
        f << "mouse.smoothing=0.5\n";
        f << "camera.index=1\n";
        f << "debug.show_window=true\n";
    }

    void TearDown() override {
        std::remove(TEST_CONFIG.c_str());
    }
};

TEST_F(ConfigManagerTest, LoadsFloatValue) {
    ConfigManager cfg(TEST_CONFIG);
    float val = cfg.get<float>("mouse.smoothing", 0.0f);
    EXPECT_NEAR(val, 0.5f, 0.001f);
}

TEST_F(ConfigManagerTest, LoadsIntValue) {
    ConfigManager cfg(TEST_CONFIG);
    int idx = cfg.get<int>("camera.index", 0);
    EXPECT_EQ(idx, 1);
}

TEST_F(ConfigManagerTest, LoadsBoolValue) {
    ConfigManager cfg(TEST_CONFIG);
    bool show = cfg.get<bool>("debug.show_window", false);
    EXPECT_TRUE(show);
}

TEST_F(ConfigManagerTest, ReturnsDefaultForMissingKey) {
    ConfigManager cfg(TEST_CONFIG);
    float val = cfg.get<float>("nonexistent.key", 99.0f);
    EXPECT_NEAR(val, 99.0f, 0.001f);
}

TEST_F(ConfigManagerTest, MissingFileUsesDefaults) {
    ConfigManager cfg("/tmp/nonexistent_file_12345.txt");
    // Не повинно кидати виключення; smoothing = 0.3 за замовчуванням
    float val = cfg.get<float>("mouse.smoothing", -1.0f);
    EXPECT_GT(val, 0.0f);
}
