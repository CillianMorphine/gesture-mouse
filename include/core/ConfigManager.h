#pragma once
#include <string>
#include <unordered_map>
#include <variant>

/**
 * @brief Менеджер конфігурації. Читає/записує JSON-файл налаштувань.
 */
class ConfigManager {
public:
    explicit ConfigManager(const std::string& filePath);

    template<typename T>
    T get(const std::string& key, const T& defaultValue) const;

    template<typename T>
    void set(const std::string& key, const T& value);

    void save() const;
    void load();

private:
    std::string m_filePath;

    // Простий key-value store (у повній версії — nlohmann::json)
    std::unordered_map<std::string, std::string> m_data;
};
