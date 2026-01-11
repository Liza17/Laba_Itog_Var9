#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <iomanip>
#include "../include/utils.hpp"


const int RECORD_COUNT = 500000;
const std::string OUTPUT_FILE = "data/example_huge.json";

std::string getRandomTimestamp() {
    int day = 1 + (rand() % 31);
    int hour = 8 + (rand() % 12); // с 08:00 до 20:00
    int min = rand() % 60;
    int sec = rand() % 60;

    char buf[30];
    sprintf_s(buf, "2025-10-%02dT%02d:%02d:%02dZ", day, hour, min, sec);
    return std::string(buf);
}

int main() {
    utils::setupConsoleEncoding();

    std::cout << "Генерация данных (" << RECORD_COUNT << " записей)...\n";

    std::vector<std::string> students = {
        "Иванов И.И.", "Петров П.П.", "Сидоров С.С.", "Смирнов А.А.",
        "Кузнецов Б.Б.", "Попов В.В.", "Васильев Г.Г.", "Михайлов Д.Д.",
        "Новиков Е.Е.", "Федоров З.З."
    };

    std::vector<std::string> types = { "in", "out", "absence" };

    std::ofstream out(utils::getPath("example_huge.json"), std::ios::binary);

    out << "[\n";
    srand(static_cast<unsigned int>(time(0)));

    for (int i = 0; i < RECORD_COUNT; ++i) {
        std::string student = students[rand() % students.size()];
        std::string type = types[rand() % types.size()];
        std::string ts = getRandomTimestamp();

        out << "  { \"student\": \"" << student << "\", "
            << "\"ts\": \"" << ts << "\", "
            << "\"type\": \"" << type << "\" }";

        if (i < RECORD_COUNT - 1) out << ",";
        out << "\n";
        if (i % 50000 == 0) std::cout << "Обработано " << i << "...\r";
    }
    out << "]";
    std::cout << "\nГотово!\n";
    return 0;
}