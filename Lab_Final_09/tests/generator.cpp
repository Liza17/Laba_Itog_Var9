#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <locale>
#include <windows.h>

namespace fs = std::filesystem;

std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), wstr.data(), size_needed);
    return wstr;
}

std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), str.data(), size_needed, nullptr, nullptr);
    return str;
}

std::wstring json_escape(const std::wstring& s) {
    std::wstring result;
    for (wchar_t c : s) {
        switch (c) {
        case L'"':  result += L"\\\""; break;
        case L'\\': result += L"\\\\"; break;
        case L'\n': result += L"\\n"; break;
        default:    result += c;
        }
    }
    return result;
}

std::wstring random_timestamp(std::mt19937& rng) {
    std::uniform_int_distribution<int> day(1, 30);
    std::uniform_int_distribution<int> hour(0, 23);
    std::uniform_int_distribution<int> minute(0, 59);
    std::uniform_int_distribution<int> second(0, 59);

    std::wstringstream oss;
    oss << L"2025-10-"
        << (day(rng) < 10 ? L"0" : L"") << day(rng)
        << L"T"
        << (hour(rng) < 10 ? L"0" : L"") << hour(rng)
        << L":"
        << (minute(rng) < 10 ? L"0" : L"") << minute(rng)
        << ":"
        << (second(rng) < 10 ? L"0" : L"") << second(rng)
        << L"Z";
    return oss.str();
}

void generate_json_file(const std::wstring& filename, int count, int error_percent = 5) {
    fs::create_directories(fs::path(filename).parent_path());

    std::wofstream out(filename);
    out.imbue(std::locale("en_US.UTF-8"));
    if (!out) {
        std::wcerr << L"Ошибка открытия файла: " << filename << L"\n";
        return;
    }

    std::vector<std::wstring> students = { L"Иванов", L"Петров", L"Сидоров", L"Смирнов", L"Кузнецов", L"Попов" };

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> student_dist(0, (int)students.size() - 1);
    std::uniform_int_distribution<int> error_chance(0, 99);
    std::uniform_int_distribution<int> type_choice(0, 99);

    out << L"[\n";
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        out << L"  {";

        if (error_chance(rng) < error_percent) {
            out << L"\"student\": null, \"ts\": \"\", \"type\": \"unknown\"";
        }
        else {
            std::wstring student = students[student_dist(rng)];
            std::wstring ts = random_timestamp(rng);

            int type_rand = type_choice(rng);
            std::wstring type;
            if (type_rand < 5) type = L"absence";   // 5% absence
            else if (type_rand < 52) type = L"in";  // 47% in
            else type = L"out";                      // 48% out

            out << L"\"student\": \"" << json_escape(student) << L"\", "
                << L"\"ts\": \"" << ts << L"\", "
                << L"\"type\": \"" << type << L"\"";
        }

        out << L"}";
        if (i < count - 1) out << L",";
        out << L"\n";

        if ((i + 1) % 10000 == 0) {
            std::wcout << filename << L": " << (i + 1) << L" записей сгенерировано\n";
        }
    }

    out << L"]";
    auto end = std::chrono::high_resolution_clock::now();
    std::wcout << filename << L" сгенерирован за "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << L" мс\n";
}

struct CLIOptions {
    std::wstring filename = L"data/example.json";
    int count = 1000;
    int error_percent = 5;
};

CLIOptions parse_cli(int argc, char* argv[]) {
    CLIOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-f" || arg == "--file") && i + 1 < argc) {
            options.filename = utf8_to_wstring(argv[++i]);
        }
        else if ((arg == "-n" || arg == "--count") && i + 1 < argc) {
            options.count = std::stoi(argv[++i]);
        }
        else if ((arg == "-e" || arg == "--error") && i + 1 < argc) {
            options.error_percent = std::stoi(argv[++i]);
        }
        else {
            std::wcerr << L"Неизвестный аргумент: " << arg.c_str() << L"\n";
            exit(1);
        }
    }
    return options;
}

int main(int argc, char* argv[]) {
    std::locale::global(std::locale("ru_RU.UTF-8"));
    std::wcout.imbue(std::locale());
    std::wcerr.imbue(std::locale());

    CLIOptions options = parse_cli(argc, argv);
    generate_json_file(options.filename, options.count, options.error_percent);

    return 0;
}
