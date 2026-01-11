#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX 
#include <windows.h>

#ifdef IN
#undef IN
#endif
#ifdef OUT
#undef OUT
#endif

#endif


namespace utils {

    const std::string DATA_DIR = "data/";

    inline void setupConsoleEncoding() {
#ifdef _WIN32
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);

#endif
    }

    inline int u8_adjust(const std::string& s, int targetWidth) {
        size_t numBytes = s.length();
        size_t numChars = 0;
        for (char c : s) {
            if ((c & 0xC0) != 0x80) {
                numChars++;
            }
        }
        return targetWidth + (int)(numBytes - numChars);
    }

    inline std::string getPath(const std::string& filename) {
        if (filename.find("data/") == 0 || filename.find("data\\") == 0) return filename;
        return DATA_DIR + filename;
    }

    inline std::string readFile(const std::string& filename) {
        std::string path = getPath(filename);
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("File not found: " + path);
        }
        std::ifstream t(path, std::ios::in | std::ios::binary);
        if (!t.is_open()) throw std::runtime_error("Cannot open file: " + path);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    inline void writeFile(const std::string& filename, const std::string& content) {
        std::string path = getPath(filename);
        if (!std::filesystem::exists(DATA_DIR)) {
            std::filesystem::create_directory(DATA_DIR);
        }
        std::ofstream t(path, std::ios::out | std::ios::binary);
        if (!t.is_open()) throw std::runtime_error("Cannot open file for writing: " + path);
        t << content;
    }

    inline size_t getFileSize(const std::string& filename) {
        try { return std::filesystem::file_size(getPath(filename)); }
        catch (...) { return 0; }
    }
}