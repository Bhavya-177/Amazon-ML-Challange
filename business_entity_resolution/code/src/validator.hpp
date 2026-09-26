#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct ValidationResult {
    float best_threshold;
    double best_macro_f05;
};

inline ValidationResult grid_search_f05(
    const std::vector<std::string>& val_s1_ids,
    const std::unordered_map<std::string, std::unordered_set<std::string>>& ground_truth,
    const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& predictions)
{
    float best_th = 0.50f;
    double max_score = -1.0;

    for (float th = 0.40f; th <= 0.90f; th += 0.02f) {
        double macro_sum = 0.0;

        for (const auto& s1_id : val_s1_ids) {
            std::unordered_set<std::string> preds;
            auto it_pred = predictions.find(s1_id);
            if (it_pred != predictions.end()) {
                for (const auto& cand : it_pred->second) {
                    if (cand.second >= th) preds.insert(cand.first);
                }
            }

            std::unordered_set<std::string> actual;
            auto it_gt = ground_truth.find(s1_id);
            if (it_gt != ground_truth.end()) actual = it_gt->second;

            // Strict singletons rule: 1.0 on empty, 0.0 on false positive
            if (actual.empty()) {
                macro_sum += (preds.empty() ? 1.0 : 0.0);
                continue;
            }
            if (preds.empty()) {
                macro_sum += 0.0;
                continue;
            }

            int tp = 0;
            for (const auto& p : preds) if (actual.count(p)) tp++;

            double prec = static_cast<double>(tp) / preds.size();
            double rec = static_cast<double>(tp) / actual.size();

            if (prec + rec == 0.0 || (0.25 * prec + rec) == 0.0) {
                macro_sum += 0.0;
            } else {
                macro_sum += (1.25 * prec * rec) / (0.25 * prec + rec);
            }
        }

        double current_score = macro_sum / static_cast<double>(val_s1_ids.size());
        if (current_score > max_score) {
            max_score = current_score;
            best_th = th;
        }
    }
    return {best_th, max_score};
}