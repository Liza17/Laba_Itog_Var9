#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>
#include "../include/simple_json.hpp"

#define TEST_CASE(name) \
    std::cout << "[RUN] " << name << "... "; \
    try

#define TEST_PASS \
    catch (const std::exception& e) { \
        std::cout << "FAIL (" << e.what() << ")\n"; \
        exit(1); \
    } \
    std::cout << "OK\n";


void test_primitives() {
    TEST_CASE("Primitives Parsing") {
        auto v1 = json::Parser::parse("true");
        assert(v1.asBool() == true);

        auto v2 = json::Parser::parse("false");
        assert(v2.asBool() == false);

        auto v3 = json::Parser::parse("null");
        assert(v3.getType() == json::Type::Null);

        auto v4 = json::Parser::parse("123.456");
        assert(std::abs(v4.asNumber() - 123.456) < 1e-6);

        auto v5 = json::Parser::parse("-100");
        assert(std::abs(v5.asNumber() + 100.0) < 1e-6);

        auto v6 = json::Parser::parse("\"Hello World\"");
        assert(v6.asString() == "Hello World");
    } TEST_PASS
}

void test_arrays() {
    TEST_CASE("Array Parsing") {
        std::string json = "[1, 2, 3, \"four\"]";
        auto root = json::Parser::parse(json);
        assert(root.getType() == json::Type::Array);

        const auto& arr = root.asArray();
        assert(arr.size() == 4);
        assert(arr[0].asNumber() == 1.0);
        assert(arr[3].asString() == "four");
    } TEST_PASS
}

void test_objects() {
    TEST_CASE("Object Parsing") {
        std::string json = "{\"name\": \"Ivan\", \"age\": 20, \"active\": true}";
        auto root = json::Parser::parse(json);
        assert(root.getType() == json::Type::Object);

        const auto& obj = root.asObject();
        assert(obj.at("name").asString() == "Ivan");
        assert(obj.at("age").asNumber() == 20.0);
        assert(obj.at("active").asBool() == true);
    } TEST_PASS
}

void test_nested() {
    TEST_CASE("Nested Structures") {
        std::string json = "{\"users\": [{\"id\": 1}, {\"id\": 2}]}";
        auto root = json::Parser::parse(json);
        auto users = root.asObject().at("users").asArray();
        assert(users.size() == 2);
        assert(users[0].asObject().at("id").asNumber() == 1.0);
    } TEST_PASS
}

void test_escaping() {
    TEST_CASE("String Escaping") {
        std::string json = "\"Line1\\nLine2\\\"Quote\\\"\"";
        auto root = json::Parser::parse(json);
        std::string res = root.asString();
        assert(res == "Line1\nLine2\"Quote\"");
    } TEST_PASS
}

void test_errors() {
    TEST_CASE("Error Handling") {
        bool caught = false;
        try {
            json::Parser::parse("{\"broken\": }");
        }
        catch (...) {
            caught = true;
        }
        assert(caught);

        caught = false;
        try {
            json::Parser::parse("[1, 2");
        }
        catch (...) {
            caught = true;
        }
        assert(caught);
    } TEST_PASS
}

int main() {
    std::cout << "=== Running Parser Tests ===\n";
    test_primitives();
    test_arrays();
    test_objects();
    test_nested();
    test_escaping();
    test_errors();
    std::cout << "=== All Tests Passed ===\n";
    return 0;
}