#include "core/ConfigManager.h"
#include "utils/Logger.h"
#include <fstream>
#include <sstream>

ConfigManager::ConfigManager(const std::string& filePath)
    : m_filePath(filePath) {
    load();
}

void ConfigManager::load() {
    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        LOG_WARN("Config file not found: " + m_filePath + ". Using defaults.");
        // Значення за замовчуванням
        m_data["mouse.smoothing"]     = "0.3";
        m_data["gesture.confidence"]  = "0.6";
        m_data["camera.index"]        = "0";
        m_data["debug.show_window"]   = "false";
        return;
    }

    // Простий парсинг "key=value"
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            m_data[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }
    LOG_INFO("Config loaded from: " + m_filePath);
}

void ConfigManager::save() const {
    std::ofstream file(m_filePath);
    for (const auto& [k, v] : m_data)
        file << k << "=" << v << "\n";
    LOG_INFO("Config saved to: " + m_filePath);
}

template<>
float ConfigManager::get<float>(const std::string& key, const float& def) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    try { return std::stof(it->second); } catch (...) { return def; }
}

template<>
int ConfigManager::get<int>(const std::string& key, const int& def) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

template<>
bool ConfigManager::get<bool>(const std::string& key, const bool& def) const {
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    return it->second == "true" || it->second == "1";
}
