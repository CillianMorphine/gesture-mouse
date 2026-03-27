#pragma once
/**
 * @file ConfigManager.h
 * @brief Менеджер конфігурації у форматі key=value.
 * @author Ваше Ім'я
 * @date 2025
 *
 * @details
 * ### Формат конфіг-файлу
 * ```ini
 * # Коментар
 * camera.index=0
 * mouse.smoothing=0.3
 * debug.show_window=false
 * ```
 *
 * ### Ієрархія ключів
 * Рекомендований формат: `module.parameter`
 *
 * | Модуль | Ключі |
 * |--------|-------|
 * | `camera.*` | index |
 * | `mouse.*` | smoothing |
 * | `gesture.*` | confidence |
 * | `debug.*` | show_window |
 */

#include <string>
#include <unordered_map>

/**
 * @brief Зчитує та зберігає налаштування у форматі key=value.
 *
 * @details
 * Підтримує типізований доступ через шаблонний метод get<T>().
 * При відсутності ключа або помилці парсингу повертає defaultValue.
 *
 * @par Приклад:
 * @code{.cpp}
 * ConfigManager cfg("config/settings.txt");
 * float smooth = cfg.get<float>("mouse.smoothing", 0.3F);
 * int   camIdx = cfg.get<int>("camera.index", 0);
 * bool  debug  = cfg.get<bool>("debug.show_window", false);
 * @endcode
 */
class ConfigManager {
public:
    /**
     * @brief Конструктор. Негайно завантажує конфіг з файлу.
     * @param filePath Шлях до конфіг-файлу (відносний або абсолютний).
     * @note Якщо файл не існує — використовуються значення за замовчуванням,
     *       без виключень.
     */
    explicit ConfigManager(const std::string& filePath);

    /**
     * @brief Повертає значення ключа, перетворене до типу T.
     *
     * @tparam T Цільовий тип: float, int, bool, std::string.
     * @param key Ключ у форматі "module.parameter".
     * @param defaultValue Значення, яке повертається при відсутності ключа.
     * @return Значення типу T або defaultValue.
     *
     * @note Спеціалізації для float, int, bool визначені в .cpp.
     *       Для std::string — просто повертає рядок.
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const;

    /**
     * @brief Встановлює значення ключа (без збереження на диск).
     * @param key Ключ.
     * @param value Нове значення (рядок).
     * @note Для збереження викличте save().
     */
    template<typename T>
    void set(const std::string& key, const T& value);

    /**
     * @brief Зберігає поточні налаштування у файл.
     * @note Перезаписує файл повністю (коментарі не зберігаються).
     */
    void save() const;

    /**
     * @brief Перезавантажує конфіг з файлу (скидає runtime-зміни).
     */
    void load();

private:
    std::string m_filePath; ///< Шлях до конфіг-файлу
    std::unordered_map<std::string, std::string> m_data; ///< Словник key→value
};
