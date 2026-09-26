#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "types.hpp"

class OpenCountryBlocker {
public:
    std::unordered_map<std::string, std::vector<int>> prefix_index;
    std::unordered_map<std::string, std::vector<int>> country_index;

    std::string get_key(const Record& r) const {
        auto chars = get_utf8_chars(r.business_name);
        if (chars.size() < 2) return r.country + "#_short_";
        return r.country + "#" + chars[0] + chars[1];
    }

    void build(const std::vector<Record>& s2_s3_pool) {
        prefix_index.clear();
        country_index.clear();
        for (int i = 0; i < static_cast<int>(s2_s3_pool.size()); ++i) {
            const auto& r = s2_s3_pool[i];
            prefix_index[get_key(r)].push_back(i);
            country_index[r.country].push_back(i);
        }
    }

    std::vector<int> retrieve_candidates(const Record& s1, int max_candidates = 25) const {
        std::vector<int> candidates;
        std::string key = get_key(s1);

        auto it = prefix_index.find(key);
        if (it != prefix_index.end()) {
            const auto& bucket = it->second;
            size_t count = std::min(bucket.size(), static_cast<size_t>(max_candidates));
            candidates.insert(candidates.end(), bucket.begin(), bucket.begin() + count);
        }

        // Open-set fallback within same country
        if (candidates.size() < 5) {
            auto c_it = country_index.find(s1.country);
            if (c_it != country_index.end()) {
                for (int idx : c_it->second) {
                    if (static_cast<int>(candidates.size()) >= max_candidates) break;
                    if (std::find(candidates.begin(), candidates.end(), idx) == candidates.end()) {
                        candidates.push_back(idx);
                    }
                }
            }
        }
        return candidates;
    }
};