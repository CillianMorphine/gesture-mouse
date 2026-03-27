/**
 * @file main.cpp
 * @brief Точка входу GestureMouse.
 *
 * Відповідає за:
 *  - парсинг аргументів командного рядка (зокрема --log-level);
 *  - ініціалізацію системи логування;
 *  - верхньорівневе перехоплення та логування всіх необроблених виключень;
 *  - запуск та коректне завершення основного циклу програми.
 *
 * ### Аргументи командного рядка
 * | Аргумент              | Опис                                             |
 * |-----------------------|--------------------------------------------------|
 * | --log-level=\<рівень\>| Встановити мінімальний рівень логування         |
 * | --lang=uk\|en         | Мова повідомлень для користувача (uk за замовч.) |
 * | --help                | Показати довідку та вийти                        |
 *
 * ### Змінні оточення
 * | Змінна              | Опис                                              |
 * |---------------------|---------------------------------------------------|
 * | GESTURE_LOG_LEVEL   | Мінімальний рівень логування (debug/info/warn...) |
 *
 * @note  Рівень логування визначається БЕЗ перекомпіляції.
 *        Пріоритет: --log-level > gesture_mouse.conf > GESTURE_LOG_LEVEL.
 */

#include "utils/logger.hpp"
#include "utils/errors.hpp"
// #include "core/application.hpp"  // розкоментувати, коли Application буде реалізовано

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ─── Хелпер: парсинг аргументів ──────────────────────────────────────────────

struct CliArgs {
    std::string log_level = "info";
    std::string lang      = "uk";
    bool        help      = false;
};

static CliArgs parseCli(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            args.help = true;
        } else if (a.rfind("--log-level=", 0) == 0) {
            args.log_level = a.substr(12);
        } else if (a.rfind("--lang=", 0) == 0) {
            args.lang = a.substr(7);
        } else {
            std::cerr << "[WARNING] Невідомий аргумент: " << a << "\n";
        }
    }
    return args;
}

static void printHelp(const char* prog) {
    std::cout
        << "GestureMouse — керування курсором жестами рук\n\n"
        << "Використання:\n"
        << "  " << prog << " [параметри]\n\n"
        << "Параметри:\n"
        << "  --log-level=LEVEL   Рівень логування: trace|debug|info|warn|error|critical\n"
        << "                      (також: змінна оточення GESTURE_LOG_LEVEL)\n"
        << "  --lang=LANG         Мова повідомлень: uk (за замовчуванням) | en\n"
        << "  --help              Показати цю довідку\n\n"
        << "Приклади:\n"
        << "  " << prog << " --log-level=debug\n"
        << "  GESTURE_LOG_LEVEL=warn " << prog << "\n";
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // 1. Парсинг CLI
    CliArgs cli = parseCli(argc, argv);

    if (cli.help) {
        printHelp(argv[0]);
        return EXIT_SUCCESS;
    }

    // 2. Мова повідомлень
    gm::setLocale(cli.lang == "en" ? gm::Locale::EN : gm::Locale::UK);

    // 3. Ініціалізація логування
    //    Рівень з CLI вже в cli.log_level; logger.cpp додатково перевірить
    //    gesture_mouse.conf та GESTURE_LOG_LEVEL.
    gm::LoggerConfig logCfg;
    logCfg.level       = cli.log_level;
    logCfg.log_dir     = "logs";
    logCfg.log_file    = "gesture_mouse.log";
    logCfg.max_file_mb = 5;
    logCfg.max_files   = 10;
    logCfg.console     = true;

    gm::initLogger(logCfg);

    // 4. Верхньорівневий try/catch — гарантуємо логування будь-якого збою
    try {
        GM_INFO("Запуск головного циклу GestureMouse...");

        // ── Тут буде викликатися Application::run() ──────────────────────
        // gm::Application app;
        // app.run();
        // ─────────────────────────────────────────────────────────────────

        // Демонстраційне логування (видалити, коли Application реалізовано)
        GM_DEBUG("Ініціалізація модулів...");
        GM_INFO("Модуль камери: OK");
        GM_INFO("Модуль жестів: OK");
        GM_INFO("Модуль введення: OK");
        GM_WARN("HSV-діапазон не налаштований, використовуються значення за замовчуванням");
        GM_INFO("Очікування жестів... (Ctrl+C для виходу)");

    } catch (const gm::GestureMouseError& e) {
        // ── Відомі помилки GestureMouse ──────────────────────────────────
        // Технічний запис вже зроблений макросом GM_THROW всередині
        // Тут додаємо повідомлення для користувача
        std::cerr << "\n╔══════════════════════════════════════════════╗\n";
        std::cerr << "║         GestureMouse: Критична помилка       ║\n";
        std::cerr << "╚══════════════════════════════════════════════╝\n\n";
        std::cerr << e.userMessage() << "\n\n";
        std::cerr << "Ідентифікатор помилки: " << gm::errorCodeStr(e.code())
                  << " | Сесія: " << gm::getSessionId() << "\n";
        std::cerr << "Деталі записані у logs/gesture_mouse.log\n\n";
        std::cerr << "Якщо проблема повторюється, відправте лог-файл розробнику.\n";

        GM_CRITICAL("Необроблена GestureMouseError: {}", e.logMessage());
        gm::shutdownLogger();
        return EXIT_FAILURE;

    } catch (const std::exception& e) {
        // ── Стандартні виключення STL ─────────────────────────────────────
        GM_CRITICAL("[GM-9999] Необроблений std::exception: {} | session={} | what={}",
                    "system", gm::getSessionId(), e.what());

        std::cerr << "\nПомилка програми. Деталі у logs/gesture_mouse.log\n";
        std::cerr << "Сесія: " << gm::getSessionId() << "\n";

        gm::shutdownLogger();
        return EXIT_FAILURE;

    } catch (...) {
        // ── Абсолютно невідоме виключення ────────────────────────────────
        GM_CRITICAL("[GM-9998] Невідоме виключення (not std::exception) | session={}",
                    gm::getSessionId());

        std::cerr << "\nНевідома критична помилка. Деталі у logs/gesture_mouse.log\n";

        gm::shutdownLogger();
        return EXIT_FAILURE;
    }

    // 5. Коректне завершення
    GM_INFO("Головний цикл завершено успішно.");
    gm::shutdownLogger();
    return EXIT_SUCCESS;
}
