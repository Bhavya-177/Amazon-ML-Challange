#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <cmath>
#include "types.hpp"

struct Features {
    float char_bi_name;
    float token_name;
    float char_tri_name;
    float token_addr;
    float char_tri_addr;
    float digit_match;
    float len_diff_name;
    float len_diff_addr;
};

inline double char_ngram_jaccard(const std::string& s1, const std::string& s2, size_t n) {
    auto chars1 = get_utf8_chars(s1);
    auto chars2 = get_utf8_chars(s2);

    if (chars1.size() < n || chars2.size() < n) {
        return s1 == s2 ? 1.0 : 0.0;
    }

    std::unordered_set<std::string> set1, set2;
    for (size_t i = 0; i + n <= chars1.size(); ++i) {
        std::string g;
        for (size_t j = 0; j < n; ++j) g += chars1[i + j];
        set1.insert(g);
    }
    for (size_t i = 0; i + n <= chars2.size(); ++i) {
        std::string g;
        for (size_t j = 0; j < n; ++j) g += chars2[i + j];
        set2.insert(g);
    }

    int inter = 0;
    for (const auto& g : set1) if (set2.count(g)) inter++;
    return static_cast<double>(inter) / (set1.size() + set2.size() - inter);
}

inline double token_jaccard(const std::string& s1, const std::string& s2) {
    std::stringstream ss1(s1), ss2(s2);
    std::string token;
    std::unordered_set<std::string> set1, set2;
    while (ss1 >> token) set1.insert(token);
    while (ss2 >> token) set2.insert(token);
    if (set1.empty() && set2.empty()) return 1.0;

    int inter = 0;
    for (const auto& w : set1) if (set2.count(w)) inter++;
    return static_cast<double>(inter) / (set1.size() + set2.size() - inter);
}

inline double digit_overlap(const std::string& s1, const std::string& s2) {
    std::string d1, d2;
    for (char c : s1) if (std::isdigit(static_cast<unsigned char>(c))) d1 += c;
    for (char c : s2) if (std::isdigit(static_cast<unsigned char>(c))) d2 += c;
    if (d1.empty() && d2.empty()) return 0.5;
    if (d1.empty() || d2.empty()) return 0.0;
    return d1 == d2 ? 1.0 : 0.0;
}

inline Features extract_features(const std::string& name1, const std::string& addr1,
                                const std::string& name2, const std::string& addr2) {
    auto u1_name = get_utf8_chars(name1);
    auto u2_name = get_utf8_chars(name2);
    auto u1_addr = get_utf8_chars(addr1);
    auto u2_addr = get_utf8_chars(addr2);

    return {
        static_cast<float>(char_ngram_jaccard(name1, name2, 2)),
        static_cast<float>(token_jaccard(name1, name2)),
        static_cast<float>(char_ngram_jaccard(name1, name2, 3)),
        static_cast<float>(token_jaccard(addr1, addr2)),
        static_cast<float>(char_ngram_jaccard(addr1, addr2, 3)),
        static_cast<float>(digit_overlap(addr1, addr2)),
        static_cast<float>(std::abs(static_cast<int>(u1_name.size()) - static_cast<int>(u2_name.size()))),
        static_cast<float>(std::abs(static_cast<int>(u1_addr.size()) - static_cast<int>(u2_addr.size())))
    };
}