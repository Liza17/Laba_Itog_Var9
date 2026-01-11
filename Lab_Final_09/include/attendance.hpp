#pragma once
#include <string>
#include <vector>
#include "simple_json.hpp"

enum class EventType { IN, OUT, ABSENCE, UNKNOWN };

struct AttendanceRecord {
    std::string student;
    std::string timestamp;
    EventType type;

    long long parseTimestamp() const;
};

class AttendanceManager {
public:
    void loadFromJson(const json::Value& root);

    json::Value saveToJson() const;

    void validateData();


    void printReportByStudent(const std::string& name) const;

    void printGeneralStats() const;

    void benchmarkAggregation();

private:
    std::vector<AttendanceRecord> records;

    static EventType strToType(const std::string& s);
    static std::string typeToStr(EventType t);
};