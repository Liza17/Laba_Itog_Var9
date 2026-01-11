#include "../include/attendance.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>
#include <ctime>
#include <cmath>

constexpr double EPS = 1e-6;

// --- Helpers ---

EventType AttendanceManager::strToType(const std::string& s) {
    if (s == "in") return EventType::IN;
    if (s == "out") return EventType::OUT;
    if (s == "absence") return EventType::ABSENCE;
    return EventType::UNKNOWN;
}

std::string AttendanceManager::typeToStr(EventType t) {
    switch (t) {
    case EventType::IN: return "in";
    case EventType::OUT: return "out";
    case EventType::ABSENCE: return "absence";
    default: return "unknown";
    }
}

long long AttendanceRecord::parseTimestamp() const {
    int year, month, day, hour, minute, second;

    if (sscanf_s(timestamp.c_str(), "%d-%d-%dT%d:%d:%d",
        &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }

    if (year < 1900 || year > 2100) return 0;
    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > 31) return 0;
    if (hour < 0 || hour > 23) return 0;
    if (minute < 0 || minute > 59) return 0;
    if (second < 0 || second > 59) return 0;

    int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) {
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (day > (isLeap ? 29 : 28)) return 0;
    }
    else if (day > daysInMonth[month - 1]) {
        return 0;
    }

    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;

    return static_cast<long long>(mktime(&tm));
}

// --- Manager ---

void AttendanceManager::loadFromJson(const json::Value& root) {
    if (root.getType() != json::Type::Array) {
        throw std::runtime_error("Root JSON must be an array");
    }

    const auto& arr = root.asArray();
    records.clear();
    records.reserve(arr.size());

    for (const auto& item : arr) {
        if (item.getType() != json::Type::Object) {
            std::cerr << "Warning: Skipping non-object element in array\n";
            continue;
        }

        const auto& obj = item.asObject();
        AttendanceRecord rec;

        auto studentIt = obj.find("student");
        if (studentIt != obj.end() && studentIt->second.getType() == json::Type::String) {
            rec.student = studentIt->second.asString();
        }
        else {
            rec.student = "Unknown";
        }

        auto tsIt = obj.find("ts");
        if (tsIt != obj.end() && tsIt->second.getType() == json::Type::String) {
            rec.timestamp = tsIt->second.asString();
        }
        else {
            rec.timestamp = "1970-01-01T00:00:00Z";
        }

        auto typeIt = obj.find("type");
        if (typeIt != obj.end() && typeIt->second.getType() == json::Type::String) {
            rec.type = strToType(typeIt->second.asString());
        }
        else {
            rec.type = EventType::UNKNOWN;
        }

        records.push_back(rec);
    }

    std::cout << "Loaded " << records.size() << " records from JSON.\n";
}

json::Value AttendanceManager::saveToJson() const {
    json::ArrayType arr;
    arr.reserve(records.size());

    for (const auto& rec : records) {
        json::ObjectType obj;
        obj["student"] = json::Value(rec.student);
        obj["ts"] = json::Value(rec.timestamp);
        obj["type"] = json::Value(typeToStr(rec.type));
        arr.push_back(json::Value(std::move(obj)));
    }

    return json::Value(std::move(arr));
}

void AttendanceManager::validateData() {
    std::cout << "Validating " << records.size() << " records...\n";

    size_t initialCount = records.size();
    size_t invalidCount = 0;

    auto it = records.begin();
    while (it != records.end()) {
        bool invalid = false;

        if (it->student.empty() || it->student == "Unknown") {
            invalid = true;
        }

        if (it->type == EventType::UNKNOWN) {
            invalid = true;
        }

        if (it->timestamp.length() < 19) {
            invalid = true;
        }

        if (it->parseTimestamp() == 0) {
            invalid = true;
        }

        if (invalid) {
            it = records.erase(it);
            invalidCount++;
        }
        else {
            ++it;
        }
    }

    std::cout << "Validation complete. Removed " << invalidCount
        << " invalid records (" << records.size() << " valid remain).\n";
}

void AttendanceManager::printReportByStudent(const std::string& name) const {
    std::cout << "\n=== Отчет для студента: " << name << " ===\n";
    std::cout << std::left
        << std::setw(25) << "Время (UTC)"
        << std::setw(10) << "Тип"
        << "\n";
    std::cout << std::string(35, '-') << "\n";

    std::vector<const AttendanceRecord*> filtered;
    filtered.reserve(records.size() / 10); // Предполагаем ~10% записей на студента

    for (const auto& rec : records) {
        if (rec.student == name) {
            filtered.push_back(&rec);
        }
    }

    if (filtered.empty()) {
        std::cout << "Записей не найдено.\n";
        return;
    }

    std::sort(filtered.begin(), filtered.end(),
        [](const AttendanceRecord* a, const AttendanceRecord* b) {
            return a->parseTimestamp() < b->parseTimestamp();
        });

    for (const auto& recPtr : filtered) {
        std::cout << std::left
            << std::setw(25) << recPtr->timestamp
            << std::setw(10) << typeToStr(recPtr->type)
            << "\n";
    }

    std::cout << "\nВсего записей: " << filtered.size() << "\n";
}

void AttendanceManager::printGeneralStats() const {
    struct StudentStat {
        int absences = 0;
        double hoursPresent = 0.0;
        long long lastIn = -1;
        int totalRecords = 0;
    };

    std::cout << "\n=== Общая статистика посещаемости ===\n";
    std::cout << "Всего студентов: ";

    std::map<std::string, StudentStat> stats;

    for (const auto& rec : records) {
        auto& stat = stats[rec.student];
        stat.totalRecords++;

        long long timestamp = rec.parseTimestamp();
        if (timestamp == 0) continue;

        switch (rec.type) {
        case EventType::ABSENCE:
            stat.absences++;
            stat.lastIn = -1;
            break;

        case EventType::IN:
            stat.lastIn = timestamp;
            break;

        case EventType::OUT:
            if (stat.lastIn != -1) {
                double diffHours = static_cast<double>(timestamp - stat.lastIn) / 3600.0;
                if (diffHours > EPS) {
                    stat.hoursPresent += diffHours;
                }
                stat.lastIn = -1;
            }
            break;

        default:
            break;
        }
    }

    std::string h1 = "Студент";
    std::string h2 = "Прогулы";
    std::string h3 = "Часов";
    std::string h4 = "Записей";

    std::cout << stats.size() << "\n\n";
    std::cout << std::left
        << std::setw(utils::u8_adjust(h1, 25)) << h1
        << std::setw(utils::u8_adjust(h2, 15)) << h2
        << std::setw(utils::u8_adjust(h3, 15)) << h3
        << std::setw(utils::u8_adjust(h4, 10)) << h4 << "\n";

    std::cout << std::string(65, '-') << "\n";

    for (const auto& [student, stat] : stats) {
        std::cout << std::left
            << std::setw(utils::u8_adjust(student, 25)) << student
            << std::setw(15) << stat.absences
            << std::setw(15) << std::fixed << std::setprecision(2) << stat.hoursPresent
            << std::setw(10) << stat.totalRecords << "\n";
    }

    int totalAbsences = 0;
    double totalHours = 0.0;
    int totalRecords = 0;

    for (const auto& [_, stat] : stats) {
        totalAbsences += stat.absences;
        totalHours += stat.hoursPresent;
        totalRecords += stat.totalRecords;
    }

    std::cout << std::string(65, '-') << "\n";
    std::cout << std::left
        << std::setw(utils::u8_adjust("Итого:", 25)) << "Итого:"
        << std::setw(15) << totalAbsences
        << std::setw(15) << std::fixed << std::setprecision(2) << totalHours
        << std::setw(10) << totalRecords << "\n";
}

void AttendanceManager::benchmarkAggregation() {
    if (records.empty()) {
        std::cout << "Нет данных для бенчмарка.\n";
        return;
    }

    std::cout << "\n=== Бенчмарк агрегации ===\n";
    std::cout << "Количество записей: " << records.size() << "\n";

    // Замер 1: Группировка и сортировка
    auto start = std::chrono::high_resolution_clock::now();

    std::map<std::string, std::vector<const AttendanceRecord*>> grouped;
    for (const auto& rec : records) {
        grouped[rec.student].push_back(&rec);
    }

    for (auto& [student, studentRecords] : grouped) {
        std::sort(studentRecords.begin(), studentRecords.end(),
            [](const AttendanceRecord* a, const AttendanceRecord* b) {
                return a->parseTimestamp() < b->parseTimestamp();
            });
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "[Benchmark] Группировка и сортировка: "
        << duration.count() << " ms\n";
    std::cout << "Записей в секунду: "
        << (records.size() * 1000.0 / duration.count()) << "\n";

    // Замер 2: Подсчёт статистики
    start = std::chrono::high_resolution_clock::now();

    int totalAbsences = 0;
    for (const auto& rec : records) {
        if (rec.type == EventType::ABSENCE) {
            totalAbsences++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "[Benchmark] Подсчёт прогулов: "
        << duration.count() << " ms\n";
    std::cout << "Прогулов всего: " << totalAbsences << "\n";

    std::cout << "=== Бенчмарк завершён ===\n";
}