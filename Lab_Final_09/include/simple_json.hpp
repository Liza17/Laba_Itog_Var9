#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <cstddef>

namespace json {

    enum class Type {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    struct Value;

    using NullType = std::monostate;
    using BoolType = bool;
    using NumberType = double;
    using StringType = std::string;
    using ArrayType = std::vector<Value>;
    using ObjectType = std::map<std::string, Value>;

    struct Value {
        std::variant<NullType, BoolType, NumberType,
            StringType, ArrayType, ObjectType> data;

        Value() : data(NullType{}) {}
        Value(bool v) : data(v) {}
        Value(double v) : data(v) {}
        Value(int v) : data(static_cast<double>(v)) {}
        Value(const std::string& v) : data(v) {}
        Value(const char* v) : data(std::string(v)) {}
        Value(const ArrayType& v) : data(v) {}
        Value(const ObjectType& v) : data(v) {}

        Type getType() const;

        std::string asString() const;
        double asNumber() const;
        bool asBool() const;
        const ArrayType& asArray() const;
        const ObjectType& asObject() const;

        ArrayType& asArray();
        ObjectType& asObject();
    };

    class Parser {
    public:
        static Value parse(const std::string& content);

        static std::string stringify(const Value& value, int indent = -1);

    private:
        static Value parseValue(const std::string& str, size_t& pos);
        static Value parseObject(const std::string& str, size_t& pos);
        static Value parseArray(const std::string& str, size_t& pos);
        static Value parseString(const std::string& str, size_t& pos);
        static Value parseNumber(const std::string& str, size_t& pos);
        static Value parseKeyword(const std::string& str, size_t& pos,
            const std::string& keyword, const Value& val);
    };

}