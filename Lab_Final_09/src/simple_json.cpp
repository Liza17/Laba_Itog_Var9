#include "../include/simple_json.hpp"
#include <cctype>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace json {

    //  Value

    Type Value::getType() const {
        if (std::holds_alternative<NullType>(data)) return Type::Null;
        if (std::holds_alternative<BoolType>(data)) return Type::Boolean;
        if (std::holds_alternative<NumberType>(data)) return Type::Number;
        if (std::holds_alternative<StringType>(data)) return Type::String;
        if (std::holds_alternative<ArrayType>(data)) return Type::Array;
        if (std::holds_alternative<ObjectType>(data)) return Type::Object;
        throw std::runtime_error("Unknown type in Value");
    }

    std::string Value::asString() const {
        if (auto* s = std::get_if<StringType>(&data)) {
            return *s;
        }
        throw std::runtime_error("Value is not a string");
    }

    double Value::asNumber() const {
        if (auto* n = std::get_if<NumberType>(&data)) {
            return *n;
        }
        throw std::runtime_error("Value is not a number");
    }

    bool Value::asBool() const {
        if (auto* b = std::get_if<BoolType>(&data)) {
            return *b;
        }
        throw std::runtime_error("Value is not a boolean");
    }

    const ArrayType& Value::asArray() const {
        if (auto* arr = std::get_if<ArrayType>(&data)) {
            return *arr;
        }
        throw std::runtime_error("Value is not an array");
    }

    const ObjectType& Value::asObject() const {
        if (auto* obj = std::get_if<ObjectType>(&data)) {
            return *obj;
        }
        throw std::runtime_error("Value is not an object");
    }

    ArrayType& Value::asArray() {
        if (auto* arr = std::get_if<ArrayType>(&data)) {
            return *arr;
        }
        throw std::runtime_error("Value is not an array");
    }

    ObjectType& Value::asObject() {
        if (auto* obj = std::get_if<ObjectType>(&data)) {
            return *obj;
        }
        throw std::runtime_error("Value is not an object");
    }


    static void skipWhitespace(const std::string& str, size_t& pos) {
        while (pos < str.size() && std::isspace(str[pos])) {
            pos++;
        }
    }

    // Parser

    Value Parser::parse(const std::string& content) {
        size_t pos = 0;
        skipWhitespace(content, pos);

        Value result = parseValue(content, pos);

        skipWhitespace(content, pos);
        if (pos < content.size()) {
            throw std::runtime_error("Unexpected characters after JSON");
        }

        return result;
    }

    Value Parser::parseValue(const std::string& str, size_t& pos) {
        skipWhitespace(str, pos);

        if (pos >= str.size()) {
            throw std::runtime_error("Unexpected end of JSON");
        }

        char c = str[pos];

        if (c == '{') return parseObject(str, pos);
        if (c == '[') return parseArray(str, pos);
        if (c == '"') return parseString(str, pos);
        if (c == 't') return parseKeyword(str, pos, "true", Value(true));
        if (c == 'f') return parseKeyword(str, pos, "false", Value(false));
        if (c == 'n') return parseKeyword(str, pos, "null", Value());
        if (isdigit(c) || c == '-') return parseNumber(str, pos);

        throw std::runtime_error("Unexpected character at position " +
            std::to_string(pos) + ": '" + c + "'");
    }

    Value Parser::parseObject(const std::string& str, size_t& pos) {
        if (str[pos] != '{') {
            throw std::runtime_error("Expected '{'");
        }
        pos++;

        ObjectType obj;
        bool first = true;

        while (true) {
            skipWhitespace(str, pos);

            if (pos >= str.size()) {
                throw std::runtime_error("Unclosed object");
            }

            if (str[pos] == '}') {
                pos++;
                break;
            }

            if (!first) {
                if (str[pos] != ',') {
                    throw std::runtime_error("Expected ',' in object");
                }
                pos++;
                skipWhitespace(str, pos);
            }
            first = false;

            if (str[pos] != '"') {
                throw std::runtime_error("Object key must be a string");
            }
            std::string key = parseString(str, pos).asString();

            skipWhitespace(str, pos);
            if (pos >= str.size() || str[pos] != ':') {
                throw std::runtime_error("Expected ':' after object key");
            }
            pos++;

            skipWhitespace(str, pos);
            Value value = parseValue(str, pos);

            obj[key] = value;
        }

        return Value(obj);
    }

    Value Parser::parseArray(const std::string& str, size_t& pos) {
        if (str[pos] != '[') {
            throw std::runtime_error("Expected '['");
        }
        pos++;

        ArrayType arr;
        bool first = true;

        while (true) {
            skipWhitespace(str, pos);

            if (pos >= str.size()) {
                throw std::runtime_error("Unclosed array");
            }

            if (str[pos] == ']') {
                pos++;
                break;
            }

            if (!first) {
                if (str[pos] != ',') {
                    throw std::runtime_error("Expected ',' in array");
                }
                pos++;
                skipWhitespace(str, pos);
            }
            first = false;

            Value element = parseValue(str, pos);
            arr.push_back(element);
        }

        return Value(arr);
    }

    Value Parser::parseString(const std::string& str, size_t& pos) {
        if (str[pos] != '"') {
            throw std::runtime_error("Expected '\"'");
        }
        pos++;

        std::string result;

        while (pos < str.size() && str[pos] != '"') {
            if (str[pos] == '\\') {
                pos++;
                if (pos >= str.size()) {
                    throw std::runtime_error("Unterminated escape sequence");
                }

                switch (str[pos]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u':
                    result += "\\u";
                    break;
                default:
                    result += str[pos];
                    break;
                }
            }
            else {
                result += str[pos];
            }
            pos++;
        }

        if (pos >= str.size() || str[pos] != '"') {
            throw std::runtime_error("Unclosed string");
        }
        pos++;

        return Value(result);
    }

    Value Parser::parseNumber(const std::string& str, size_t& pos) {
        size_t start = pos;

        if (str[pos] == '-') {
            pos++;
        }

        if (isdigit(str[pos])) {
            while (pos < str.size() && isdigit(str[pos])) {
                pos++;
            }
        }
        else {
            throw std::runtime_error("Expected digit in number");
        }

        if (pos < str.size() && str[pos] == '.') {
            pos++;
            if (!isdigit(str[pos])) {
                throw std::runtime_error("Expected digit after decimal point");
            }
            while (pos < str.size() && isdigit(str[pos])) {
                pos++;
            }
        }

        if (pos < str.size() && (str[pos] == 'e' || str[pos] == 'E')) {
            pos++;
            if (str[pos] == '+' || str[pos] == '-') {
                pos++;
            }
            if (!isdigit(str[pos])) {
                throw std::runtime_error("Expected digit in exponent");
            }
            while (pos < str.size() && isdigit(str[pos])) {
                pos++;
            }
        }

        std::string numStr = str.substr(start, pos - start);
        try {
            double num = std::stod(numStr);
            return Value(num);
        }
        catch (...) {
            throw std::runtime_error("Invalid number format: " + numStr);
        }
    }

    Value Parser::parseKeyword(const std::string& str, size_t& pos,
        const std::string& keyword, const Value& val) {
        for (size_t i = 0; i < keyword.size(); i++) {
            if (pos >= str.size() || str[pos] != keyword[i]) {
                throw std::runtime_error("Expected keyword '" + keyword + "'");
            }
            pos++;
        }
        return val;
    }

    std::string Parser::stringify(const Value& value, int indent) {
        switch (value.getType()) {
        case Type::Null:
            return "null";

        case Type::Boolean:
            return value.asBool() ? "true" : "false";

        case Type::Number: {
            double num = value.asNumber();
            std::stringstream ss;
            if (num == static_cast<long long>(num)) {
                ss << static_cast<long long>(num);
            }
            else {
                ss << std::fixed << std::setprecision(15) << num;
                std::string s = ss.str();
                while (s.back() == '0') {
                    s.pop_back();
                }
                if (s.back() == '.') {
                    s.pop_back();
                }
                return s;
            }
            return ss.str();
        }

        case Type::String: {
            std::string str = value.asString();
            std::string result = "\"";

            for (char c : str) {
                switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c >= 0 && c < 32) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    }
                    else {
                        result += c;
                    }
                    break;
                }
            }

            result += "\"";
            return result;
        }

        case Type::Array: {
            const ArrayType& arr = value.asArray();
            std::string result = "[";

            for (size_t i = 0; i < arr.size(); i++) {
                if (i > 0) {
                    result += ",";
                }
                result += stringify(arr[i], indent);
            }

            result += "]";
            return result;
        }

        case Type::Object: {
            const ObjectType& obj = value.asObject();
            std::string result = "{";
            bool first = true;

            for (const auto& [key, val] : obj) {
                if (!first) {
                    result += ",";
                }
                first = false;

                result += "\"" + key + "\":";
                result += stringify(val, indent);
            }

            result += "}";
            return result;
        }
        }

        return "null";
    }

}