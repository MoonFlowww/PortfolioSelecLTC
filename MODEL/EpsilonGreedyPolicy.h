#pragma once
#include <random>
#include <vector>
#include <algorithm>

class EpsilonGreedyPolicy {
public:
    double epsilon;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

    EpsilonGreedyPolicy(double eps) : epsilon(eps), gen(std::random_device{}()), dis(0.0, 1.0) {}

    int select_action(const std::vector<double>& action_values) {
        if (dis(gen) < epsilon) {
            // Exploration: rand action
            std::uniform_int_distribution<> action_dist(0, action_values.size() - 1);
            return action_dist(gen);
        }
        else {
            // Exploitation: highest value
            return std::distance(action_values.begin(), std::max_element(action_values.begin(), action_values.end()));
        }
    }

    void decay_epsilon(double decay_rate) {
        epsilon *= decay_rate;
    }
};
