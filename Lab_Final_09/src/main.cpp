#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <chrono>
#include "../include/simple_json.hpp"
#include "../include/attendance.hpp"
#include "../include/utils.hpp"

void printHelp() {
    std::cout << "Attendance CLI Tool - Учёт посещаемости\n"
        << "Использование: app [опции]\n\n"
        << "Опции:\n"
        << "  --help              Показать эту справку\n"
        << "  --input <файл>      Загрузить JSON файл при запуске\n"
        << "  --student <имя>     Показать отчёт для студента и выйти\n"
        << "  --bench             Запустить бенчмарк и выйти\n"
        << "  --validate-only     Только валидировать данные и выйти\n\n"
        << "Примеры:\n"
        << "  app --input data.json\n"
        << "  app --input data.json --student \"Иванов И.И.\"\n"
        << "  app --input data.json --bench\n";
}

bool askConfirmation(const std::string& message) {
    std::cout << message << " (y/n): ";
    char response;
    std::cin >> response;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return (response == 'y' || response == 'Y');
}

void interactiveMenu(AttendanceManager& manager) {
    while (true) {
        std::cout << "\n=== Меню управления ===\n";
        std::cout << "1. Общая статистика\n";
        std::cout << "2. Отчёт по студенту\n";
        std::cout << "3. Сохранить данные в JSON\n";
        std::cout << "4. Запустить бенчмарк\n";
        std::cout << "5. Информация о данных\n";
        std::cout << "0. Выход\n";
        std::cout << "Выберите действие: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cout << "Ошибка: введите число.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
        case 1:
            manager.printGeneralStats();
            break;

        case 2: {
            std::cout << "Введите имя студента: ";
            std::string name;
            std::getline(std::cin, name);

            if (!name.empty() && name.back() == '\r') {
                name.pop_back();
            }

            if (!name.empty()) {
                manager.printReportByStudent(name);
            }
            else {
                std::cout << "Ошибка: пустое имя.\n";
            }
            break;
        }

        case 3: {
            std::cout << "Введите имя файла для сохранения: ";
            std::string path;
            std::getline(std::cin, path);

            if (!path.empty() && path.back() == '\r') {
                path.pop_back();
            }

            if (path.empty()) {
                std::cout << "Отменено.\n";
                break;
            }

            std::string fullPath = utils::getPath(path);
            if (std::filesystem::exists(fullPath)) {
                if (!askConfirmation("Файл уже существует. Перезаписать?")) {
                    std::cout << "Отменено.\n";
                    break;
                }
            }

            try {
                auto jsonVal = manager.saveToJson();
                std::string jsonStr = json::Parser::stringify(jsonVal, 2);
                utils::writeFile(path, jsonStr);
                std::cout << "Данные успешно сохранены в " << fullPath << "\n";
            }
            catch (const std::exception& e) {
                std::cerr << "Ошибка сохранения: " << e.what() << "\n";
            }
            break;
        }

        case 4:
            manager.benchmarkAggregation();
            break;

        case 5: {
            std::cout << "\n=== Информация о данных ===\n";
            std::cout << "Используйте опцию --bench для подробной информации.\n";
            break;
        }

        case 0:
            std::cout << "Выход из программы.\n";
            return;

        default:
            std::cout << "Ошибка: неизвестная опция.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    utils::setupConsoleEncoding();

    std::cout << "=== Attendance CLI Tool ===\n";
    std::cout << "Учёт посещаемости студентов\n\n";

    AttendanceManager manager;
    std::string inputFile = "";
    std::string targetStudent = "";
    bool runBench = false;
    bool validateOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printHelp();
            return 0;
        }
        else if (arg == "--input" && i + 1 < argc) {
            inputFile = argv[++i];
        }
        else if (arg == "--student" && i + 1 < argc) {
            targetStudent = argv[++i];
        }
        else if (arg == "--bench") {
            runBench = true;
        }
        else if (arg == "--validate-only") {
            validateOnly = true;
        }
        else {
            std::cerr << "Предупреждение: неизвестный аргумент '" << arg << "'\n";
        }
    }

    try {
        if (inputFile.empty()) {
            inputFile = "example_valid.json";
            std::cout << "Файл не указан. Используется: " << inputFile << "\n";

            if (!std::filesystem::exists(utils::getPath(inputFile))) {
                std::cerr << "Ошибка: файл " << inputFile << " не найден.\n";
                std::cout << "Создайте файл example_valid.json в папке data/\n";
                return 1;
            }
        }

        std::cout << "Загрузка файла: " << inputFile << "\n";

        std::string content;
        try {
            size_t fileSize = utils::getFileSize(inputFile);
            if (fileSize == 0) {
                std::cerr << "Ошибка: файл пустой или не существует.\n";
                return 1;
            }

            std::cout << "Размер файла: " << (fileSize / 1024) << " KB\n";

            if (fileSize > 100 * 1024 * 1024) {
                if (!askConfirmation("Файл очень большой. Продолжить?")) {
                    return 0;
                }
            }

            content = utils::readFile(inputFile);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка чтения файла: " << e.what() << "\n";
            return 1;
        }

        std::cout << "Парсинг JSON...\n";
        auto startParse = std::chrono::high_resolution_clock::now();

        json::Value root;
        try {
            root = json::Parser::parse(content);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка парсинга JSON: " << e.what() << "\n";
            return 1;
        }

        auto endParse = std::chrono::high_resolution_clock::now();
        auto parseTime = std::chrono::duration<double, std::milli>(endParse - startParse);

        std::cout << "JSON успешно распарсен за " << parseTime.count() << " мс\n";

        manager.loadFromJson(root);

        manager.validateData();

        if (validateOnly) {
            std::cout << "\nВалидация завершена. Программа завершает работу.\n";
            return 0;
        }

        if (runBench) {
            manager.benchmarkAggregation();
            return 0;
        }

        if (!targetStudent.empty()) {
            manager.printReportByStudent(targetStudent);
            return 0;
        }

        interactiveMenu(manager);
    }
    catch (const std::exception& e) {
        std::cerr << "\n!!! Критическая ошибка: " << e.what() << "\n";
        std::cerr << "Программа завершена аварийно.\n";
        return 1;
    }

    return 0;
}