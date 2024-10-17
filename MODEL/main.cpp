#include "LTC.h"
#include "DenseLayer.h"
#include "AdamOptimizer.h"
#include "EpsilonGreedyPolicy.h"
#include "FinancialData.h"
#include "CSVReader.h"
#include "DataPreprocessing.h"
#include "RewardFunction.h"
#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cmath>

std::vector<double> concat(const std::vector<double>& v1, const std::vector<double>& v2) {
    std::vector<double> result = v1;
    result.insert(result.end(), v2.begin(), v2.end());
    return result;
}

void extractInputs(const FinancialData& fd,
    std::vector<double>& inputs_macro,
    std::vector<double>& inputs_accounting,
    std::vector<double>& inputs_market) {
    // Inputs Macro
    inputs_macro.clear();
    inputs_macro.push_back(fd.interestRate);
    inputs_macro.push_back(fd.unemploymentRate);
    inputs_macro.push_back(fd.inflation);
    inputs_macro.push_back(fd.growthRate);
    inputs_macro.push_back(fd.consumerSentiment);

    // Inputs accounting
    inputs_accounting.clear();
    inputs_accounting.push_back(fd.salesFigures);
    inputs_accounting.push_back(fd.grossMargin);
    inputs_accounting.push_back(fd.selfFinancingCapacity);
    inputs_accounting.push_back(fd.netIncome);
    inputs_accounting.push_back(fd.profitPerStock);
    inputs_accounting.push_back(fd.freeCashFlow);
    inputs_accounting.push_back(fd.netDebtToEquity);
    inputs_accounting.push_back(fd.roa);
    inputs_accounting.push_back(fd.ebitda);

    // Inputs Market
    inputs_market.clear();
    inputs_market.push_back(fd.stockPrice);
    inputs_market.push_back(fd.sectorSentiment);
    inputs_market.push_back(fd.beta);
    inputs_market.push_back(fd.dividendYield);
    inputs_market.push_back(fd.sharpeRatio);
    inputs_market.push_back(fd.cagr);
    inputs_market.push_back(fd.var);
    inputs_market.push_back(fd.cvar);
}

std::vector<double> softmax(const std::vector<double>& logits) {
    std::vector<double> exp_logits(logits.size());
    double max_logit = *std::max_element(logits.begin(), logits.end());
    double sum_exp = 0.0;
    for (size_t i = 0; i < logits.size(); ++i) {
        exp_logits[i] = std::exp(logits[i] - max_logit); // stabilisation
        sum_exp += exp_logits[i];
    }
    for (size_t i = 0; i < logits.size(); ++i) {
        exp_logits[i] /= sum_exp;
    }
    return exp_logits;
}

int main() {

    std::vector<FinancialData> data = loadFinancialData("financial_data.csv");

    normalizeData(data);

    int num_units_macro = 5;         // nb neurons 
    int num_units_accounting = 5;    // nb neurons 
    int num_units_market = 5;        // nb neurons 

    int input_size_macro = 5;        // nb input macro
    int input_size_accounting = 9;   // nb accounting inputs
    int input_size_market = 8;       // nb markets inputs

    LTCCell ltc_macro(num_units_macro, input_size_macro);
    LTCCell ltc_accounting(num_units_accounting, input_size_accounting);
    LTCCell ltc_market(num_units_market, input_size_market);


    std::vector<double> state_macro(num_units_macro, 0.0);
    std::vector<double> state_accounting(num_units_accounting, 0.0);
    std::vector<double> state_market(num_units_market, 0.0);

    int combined_output_size = num_units_macro + num_units_accounting + num_units_market;
    DenseLayer final_layer(combined_output_size, 2); 

    AdamOptimizer adam(0.001, 0.9, 0.999, 1e-8);
    adam.initialize(final_layer.weights, final_layer.biases);

    EpsilonGreedyPolicy policy(0.1);  
    double epsilon_decay = 0.995;

    int epochs = 10; 
    std::unordered_map<std::string, double> cumulative_rewards;


    std::random_device rd;
    std::mt19937 g(rd());

    for (int epoch = 0; epoch < epochs; ++epoch) {
        std::shuffle(data.begin(), data.end(), g);

        for (const auto& fd : data) {
            std::vector<double> inputs_macro, inputs_accounting, inputs_market;
            extractInputs(fd, inputs_macro, inputs_accounting, inputs_market);

            std::vector<double> new_state_macro = ltc_macro(inputs_macro, state_macro);
            std::vector<double> new_state_accounting = ltc_accounting(inputs_accounting, state_accounting);
            std::vector<double> new_state_market = ltc_market(inputs_market, state_market);

            std::vector<double> combined_output = concat(new_state_macro, new_state_accounting);
            combined_output = concat(combined_output, new_state_market);


            std::vector<double> logits = final_layer.forward(combined_output);
            std::vector<double> action_probs = softmax(logits);


            int action = policy.select_action(action_probs);


            double reward = compute_reward(fd);

            cumulative_rewards[fd.symbol] += reward;


            double advantage = reward;

            std::vector<double> grad_output(action_probs.size(), 0.0);
            for (size_t i = 0; i < grad_output.size(); ++i) {
                if (i == action) {
                    grad_output[i] = -advantage * (1 - action_probs[i]);
                }
                else {
                    grad_output[i] = advantage * action_probs[i];
                }
            }

            // backprop
            std::vector<std::vector<double>> dW;
            std::vector<double> dB;
            final_layer.backward(combined_output, grad_output, dW, dB);
            adam.update(final_layer.weights, final_layer.biases, dW, dB, epoch + 1);
        }


        policy.decay_epsilon(epsilon_decay);


        if (epoch % 1 == 0) {
            std::cout << "Epoch: " << epoch << std::endl;
            for (const auto& pair : cumulative_rewards) {
                const std::string& symbol = pair.first;
                double reward = pair.second;
                std::cout << "Symbol: " << symbol << " - Cumulative Reward: " << reward << std::endl;
            }



        }
    }

    return 0;
}
