#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

struct Record {
    std::string entity_id;
    std::string business_name;
    std::string business_address;
    std::string country;
};

// Safe UTF-8 character extraction (handles Hindi Devanagari and European glyphs)
inline std::vector<std::string> get_utf8_chars(const std::string& str) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;

        if (i + len <= str.size()) {
            chars.push_back(str.substr(i, len));
            i += len;
        } else {
            break;
        }
    }
    return chars;
}

// Open-set country string normalizer: trims and lowercases dynamically
inline std::string normalize_country(const std::string& input) {
    if (input.empty()) return "unknown";
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "unknown";
    size_t end = input.find_last_not_of(" \t\r\n");

    std::string out;
    out.reserve(end - start + 1);
    for (size_t i = start; i <= end; ++i) {
        out.push_back(std::tolower(static_cast<unsigned char>(input[i])));
    }
    return out;
}

// Ingestion for pre-cleaned TSVs without splitting internal commas
inline std::vector<Record> read_tsv(const std::string& path) {
    std::vector<Record> records;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open TSV: " << path << std::endl;
        return records;
    }

    std::string line;
    std::getline(file, line); // Skip header row

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string id, name, addr, country;
        if (std::getline(ss, id, '\t') &&
            std::getline(ss, name, '\t') &&
            std::getline(ss, addr, '\t') &&
            std::getline(ss, country, '\t')) {
            records.push_back({id, name, addr, normalize_country(country)});
        }
    }
    return records;
}