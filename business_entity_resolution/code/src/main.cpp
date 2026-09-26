#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "types.hpp"
#include "features.hpp"
#include "blocking.hpp"
#include "model.hpp"
#include "validator.hpp"

std::unordered_map<std::string, std::unordered_set<std::string>> load_ground_truth(const std::string& path) {
    std::unordered_map<std::string, std::unordered_set<std::string>> gt;
    std::ifstream file(path);
    if (!file.is_open()) return gt;

    std::string line;
    std::getline(file, line); // Skip header
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string s1_id, matched_ids;
        if (std::getline(ss, s1_id, '\t')) {
            std::getline(ss, matched_ids, '\t');
            std::stringstream mss(matched_ids);
            std::string mid;
            while (std::getline(mss, mid, ',')) {
                if (!mid.empty()) gt[s1_id].insert(mid);
            }
            if (!gt.count(s1_id)) gt[s1_id] = {};
        }
    }
    return gt;
}

int main() {
    std::cout << "=== Running Business Entity Resolution Pipeline ===" << std::endl;

    // 1. Ingestion
    std::cout << "[1/6] Loading pre-cleaned training datasets..." << std::endl;
    auto train_s1 = read_tsv("dataset/cleaned/train_source1_clean.tsv");
    auto train_s2 = read_tsv("dataset/cleaned/train_source2_clean.tsv");
    auto train_s3 = read_tsv("dataset/cleaned/train_source3_clean.tsv");
    auto ground_truth = load_ground_truth("dataset/train/train_ground_truth.tsv");

    std::vector<Record> train_pool;
    train_pool.reserve(train_s2.size() + train_s3.size());
    train_pool.insert(train_pool.end(), train_s2.begin(), train_s2.end());
    train_pool.insert(train_pool.end(), train_s3.begin(), train_s3.end());

    // 2. Candidate Blocking
    std::cout << "[2/6] Building blocking index over candidate pool..." << std::endl;
    OpenCountryBlocker train_blocker;
    train_blocker.build(train_pool);

    // 3. Model Training
    std::cout << "[3/6] Generating pairwise training samples..." << std::endl;
    std::vector<Features> X;
    std::vector<int> y;

    size_t split_idx = static_cast<size_t>(train_s1.size() * 0.80);
    for (size_t i = 0; i < split_idx; ++i) {
        const auto& s1 = train_s1[i];
        auto cand_indices = train_blocker.retrieve_candidates(s1, 15);
        const auto& actual = ground_truth[s1.entity_id];

        for (int c_idx : cand_indices) {
            const auto& cand = train_pool[c_idx];
            X.push_back(extract_features(s1.business_name, s1.business_address,
                                         cand.business_name, cand.business_address));
            y.push_back(actual.count(cand.entity_id) ? 1 : 0);
        }
    }

    PairwiseModel model;
    model.train(X, y, 0.05f, 6);

    // 4. Threshold Grid Search
    std::cout << "[4/6] Evaluating holdout set for optimal F_0.5 decision threshold..." << std::endl;
    std::vector<std::string> val_ids;
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> val_predictions;

    for (size_t i = split_idx; i < train_s1.size(); ++i) {
        const auto& s1 = train_s1[i];
        val_ids.push_back(s1.entity_id);
        auto cand_indices = train_blocker.retrieve_candidates(s1, 20);

        for (int c_idx : cand_indices) {
            const auto& cand = train_pool[c_idx];
            float prob = model.predict_proba(extract_features(s1.business_name, s1.business_address,
                                                             cand.business_name, cand.business_address));
            val_predictions[s1.entity_id].push_back({cand.entity_id, prob});
        }
    }

    auto val_res = grid_search_f05(val_ids, ground_truth, val_predictions);
    std::cout << ">> Optimal Threshold: " << val_res.best_threshold
              << " | Validation Macro F_0.5: " << val_res.best_macro_f05 << std::endl;

    // 5. Test Inference
    std::cout << "[5/6] Ingesting test sets and indexing..." << std::endl;
    auto test_s1 = read_tsv("dataset/cleaned/test_source1_clean.tsv");
    auto test_s2 = read_tsv("dataset/cleaned/test_source2_clean.tsv");
    auto test_s3 = read_tsv("dataset/cleaned/test_source3_clean.tsv");

    std::vector<Record> test_pool;
    test_pool.reserve(test_s2.size() + test_s3.size());
    test_pool.insert(test_pool.end(), test_s2.begin(), test_s2.end());
    test_pool.insert(test_pool.end(), test_s3.begin(), test_s3.end());

    OpenCountryBlocker test_blocker;
    test_blocker.build(test_pool);

    // 6. Write TSV Submissions
    std::cout << "[6/6] Emitting candidate_pairs.tsv and matching_results.tsv..." << std::endl;
    std::ofstream f_cand("output/candidate_pairs.tsv");
    std::ofstream f_match("output/matching_results.tsv");

    f_cand << "source1_entity_id\tcandidate_entity_ids\n";
    f_match << "source1_entity_id\tmatched_entity_ids\n";

    for (const auto& s1 : test_s1) {
        f_cand << s1.entity_id << "\t";
        f_match << s1.entity_id << "\t";

        auto cand_indices = test_blocker.retrieve_candidates(s1, 25);
        bool first_c = true;
        bool first_m = true;

        for (int c_idx : cand_indices) {
            const auto& cand = test_pool[c_idx];
            float score = model.predict_proba(extract_features(s1.business_name, s1.business_address,
                                                              cand.business_name, cand.business_address));

            if (!first_c) f_cand << ",";
            f_cand << cand.entity_id;
            first_c = false;

            if (score >= val_res.best_threshold) {
                if (!first_m) f_match << ",";
                f_match << cand.entity_id;
                first_m = false;
            }
        }
        f_cand << "\n";
        f_match << "\n";
    }

    std::cout << "Done! Output files generated in output/ directory." << std::endl;
    return 0;
}