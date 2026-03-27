#pragma once
/**
 * @file Application.h
 * @brief Головний клас програми GestureMouse.
 * @author Ваше Ім'я
 * @date 2025
 * @copyright MIT License
 *
 * @details
 * Application реалізує патерн **Facade** — він є єдиною точкою входу,
 * яка ініціалізує, координує та завершує роботу всіх підсистем:
 *
 * ```
 * main()
 *   └─► Application::run()
 *         ├─► initialize()   — створює ConfigManager, GestureRecognizer,
 *         │                    MouseController, TrayIcon
 *         ├─► mainLoop()     — захоплює кадри, розпізнає жести,
 *         │                    передає команди мишці (~60 FPS)
 *         └─► cleanup()      — звільняє ресурси у зворотному порядку
 * ```
 *
 * ### Потокова модель
 * Вся обробка відбувається в **одному потоці** (головному).
 * Зупинку можна ініціювати з будь-якого потоку через stop(),
 * бо m_running є std::atomic<bool>.
 *
 * @see GestureRecognizer
 * @see MouseController
 * @see ConfigManager
 */

#include <atomic>
#include <memory>

// Forward declarations — уникаємо важких включень у заголовку
class GestureRecognizer; ///< Модуль розпізнавання жестів
class MouseController;   ///< Модуль керування курсором
class ConfigManager;     ///< Менеджер конфігурації
class TrayIcon;          ///< Іконка у системному треї

/**
 * @brief Головний клас програми — керує життєвим циклом усіх компонентів.
 *
 * @details
 * Клас не підлягає копіюванню (Rule of 5). Типове використання:
 *
 * @code{.cpp}
 * int main(int argc, char** argv) {
 *     Application app(argc, argv);
 *     return app.run();          // блокує до зупинки
 * }
 * @endcode
 *
 * ### Архітектурне рішення
 * Application зберігає всі залежності як `std::unique_ptr` —
 * це забезпечує чіткий порядок знищення та спрощує тестування
 * (залежності можна підміняти).
 */
class Application {
public:
    /**
     * @brief Конструктор. Виконує ініціалізацію всіх підсистем.
     *
     * @param argc Кількість аргументів командного рядка (з main).
     * @param argv Масив рядків аргументів (з main).
     *
     * @throws std::runtime_error якщо не вдалося відкрити конфіг або камеру.
     *
     * @note Передані argc/argv зберігаються як є — без копіювання рядків,
     *       тому час їхнього життя має перевищувати час життя Application.
     */
    Application(int argc, char** argv);

    /**
     * @brief Деструктор. Викликає cleanup() та зупиняє головний цикл.
     */
    ~Application();

    /// @name Некопійовані операції (Rule of 5)
    /// @{
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;
    /// @}

    /**
     * @brief Запускає головний цикл програми. Блокує до виклику stop().
     *
     * @details
     * Послідовність виконання:
     * 1. Відображає іконку в системному треї.
     * 2. Входить у нескінченний цикл: `захват кадру → розпізнавання → дія`.
     * 3. Повертає керування після того, як m_running стає false.
     *
     * @return EXIT_SUCCESS (0) при нормальному завершенні,
     *         EXIT_FAILURE (1) якщо камеру не вдалося відкрити.
     *
     * @note Викличте stop() з іншого потоку або з обробника сигналу,
     *       щоб завершити цикл.
     *
     * @see stop()
     */
    [[nodiscard]] int run();

    /**
     * @brief Надсилає сигнал завершення головному циклу.
     *
     * @details
     * Метод є потокобезпечним завдяки `std::atomic<bool>`.
     * Цикл завершується після поточної ітерації (не миттєво).
     *
     * Типові сценарії виклику:
     * - Клік "Вихід" у системному треї → TrayIcon::onExit()
     * - Сигнал SIGINT / SIGTERM (Ctrl+C)
     * - Команда --no-gui в тестовому режимі
     *
     * @note noexcept — метод ніколи не кидає виключень.
     */
    void stop() noexcept;

private:
    /**
     * @brief Ініціалізує всі підсистеми у правильному порядку.
     *
     * @details
     * Порядок ініціалізації важливий через залежності:
     * 1. ConfigManager  — читає файл налаштувань
     * 2. GestureRecognizer — залежить від ConfigManager
     * 3. MouseController   — залежить від ConfigManager
     * 4. TrayIcon          — залежить від Application* (this)
     *
     * @throws std::runtime_error при критичних помилках ініціалізації.
     */
    void initialize();

    /**
     * @brief Головний цикл обробки відео.
     *
     * @details
     * **Алгоритм одної ітерації:**
     * 1. `cv::VideoCapture::read()` — захват кадру з камери
     * 2. `GestureRecognizer::process()` — розпізнавання жесту
     * 3. `MouseController::applyGesture()` — симуляція введення
     * 4. `std::this_thread::sleep_for(16ms)` — обмеження до ~60 FPS
     *
     * Цикл виконується доки `m_running == true`.
     *
     * **Вибір sleep замість WaitKey:** `cv::waitKey(16)` блокує потік
     * тільки якщо відкрите вікно OpenCV. Оскільки debug-вікно
     * опціональне, `sleep_for` є надійнішим рішенням.
     */
    void mainLoop();

    /**
     * @brief Звільняє всі ресурси у зворотному порядку до ініціалізації.
     *
     * @details
     * unique_ptr забезпечує RAII-звільнення автоматично,
     * але явний виклик reset() у правильному порядку запобігає
     * use-after-free при деструкції залежних компонентів.
     */
    void cleanup();

    /// @name Підсистеми (власні, керуються через RAII)
    /// @{
    std::unique_ptr<GestureRecognizer> m_recognizer; ///< Модуль CV-розпізнавання
    std::unique_ptr<MouseController>   m_mouseCtrl;  ///< Модуль керування мишею
    std::unique_ptr<ConfigManager>     m_config;     ///< Файл конфігурації
    std::unique_ptr<TrayIcon>          m_tray;       ///< UI — системний трей
    /// @}

    /**
     * @brief Прапорець активного стану головного циклу.
     * @details Атомарний для потокобезпечного доступу з TrayIcon-потоку.
     */
    std::atomic<bool> m_running{false};

    int    m_argc{0};     ///< Кількість аргументів CLI (збережена копія)
    char** m_argv{nullptr}; ///< Масив аргументів CLI (не власник пам'яті)
};
