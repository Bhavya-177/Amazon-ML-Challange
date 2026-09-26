#pragma once
#include <vector>
#include <cmath>
#include "features.hpp"

class PairwiseModel {
public:
    std::vector<float> weights = {2.8f, 2.2f, 2.0f, 1.6f, 1.4f, 1.0f, -0.04f, -0.01f};
    float bias = -2.6f;

    float sigmoid(float z) const {
        if (z < -15.0f) return 0.0f;
        if (z > 15.0f) return 1.0f;
        return 1.0f / (1.0f + std::exp(-z));
    }

    float predict_proba(const Features& f) const {
        float z = bias +
                  weights[0] * f.char_bi_name +
                  weights[1] * f.token_name +
                  weights[2] * f.char_tri_name +
                  weights[3] * f.token_addr +
                  weights[4] * f.char_tri_addr +
                  weights[5] * f.digit_match +
                  weights[6] * f.len_diff_name +
                  weights[7] * f.len_diff_addr;
        return sigmoid(z);
    }

    void train(const std::vector<Features>& X, const std::vector<int>& y, float lr = 0.02f, int epochs = 8) {
        for (int ep = 0; ep < epochs; ++ep) {
            for (size_t i = 0; i < X.size(); ++i) {
                float p = predict_proba(X[i]);
                float err = p - static_cast<float>(y[i]);
                weights[0] -= lr * err * X[i].char_bi_name;
                weights[1] -= lr * err * X[i].token_name;
                weights[2] -= lr * err * X[i].char_tri_name;
                weights[3] -= lr * err * X[i].token_addr;
                weights[4] -= lr * err * X[i].char_tri_addr;
                weights[5] -= lr * err * X[i].digit_match;
                weights[6] -= lr * err * X[i].len_diff_name;
                weights[7] -= lr * err * X[i].len_diff_addr;
                bias -= lr * err;
            }
        }
    }
};