#include "LTC.h"
#include "DenseLayer.h"
#include "AdamOptimizer.h"
#include "Cost.h"
#include <iostream>
#include <vector>
#include <random>

std::vector<double> concat(const std::vector<double>& v1, const std::vector<double>& v2) {
    std::vector<double> result = v1;
    result.insert(result.end(), v2.begin(), v2.end());
    return result;
}


std::vector<double> generate_dummy_data(int size, double min_val = 0.0, double max_val = 1.0) {
    std::vector<double> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min_val, max_val);

    for (int i = 0; i < size; ++i) {
        data[i] = dis(gen);
    }
    return data;
}

int main() {

    int num_units_macro = 5;
    int num_units_accounting = 5;
    int num_units_market = 5;

    int input_size_macro = 3;
    int input_size_accounting = 3;
    int input_size_market = 3;

    LTCCell ltc_macro(num_units_macro, input_size_macro);
    LTCCell ltc_accounting(num_units_accounting, input_size_accounting);
    LTCCell ltc_market(num_units_market, input_size_market);

    std::vector<double> inputs_macro = generate_dummy_data(input_size_macro, 0.0, 1.0);
    std::vector<double> inputs_accounting = generate_dummy_data(input_size_accounting, 0.0, 1.0);
    std::vector<double> inputs_market = generate_dummy_data(input_size_market, 0.0, 1.0);


    std::vector<double> state_macro(num_units_macro, 0.0);
    std::vector<double> state_accounting(num_units_accounting, 0.0);
    std::vector<double> state_market(num_units_market, 0.0);

    int combined_output_size = num_units_macro + num_units_accounting + num_units_market;
    DenseLayer final_layer(combined_output_size, 1);

    AdamOptimizer adam(0.001, 0.9, 0.999, 1e-8);
    adam.initialize(final_layer.weights, final_layer.biases);
    std::vector<double> combined_output;




    EpsilonGreedyPolicy policy(0.1);
    double epsilon_decay = 0.995;

    int epochs = 1000;
    for (int epoch = 0; epoch < epochs; ++epoch) {
        std::vector<double> new_state_macro = ltc_macro(inputs_macro, state_macro);
        std::vector<double> new_state_accounting = ltc_accounting(inputs_accounting, state_accounting);
        std::vector<double> new_state_market = ltc_market(inputs_market, state_market);


        combined_output = concat(new_state_macro, new_state_accounting);
        combined_output = concat(combined_output, new_state_market);


        std::vector<double> predictions = final_layer.forward(combined_output);
        
        double reward = compute_reward(predictions, target);
        
        // adam update
        std::vector<double> grad_output = mse_grad(predictions, target);
        std::vector<std::vector<double>> dW;
        std::vector<double> dB;
        final_layer.backward(combined_output, grad_output, dW, dB);
        adam.update(final_layer.weights, final_layer.biases, dW, dB, epoch + 1);

        // exploration little by little
        policy.decay_epsilon(epsilon_decay);

        if (epoch % 100 == 0) {
            std::cout << "Epoch: " << epoch << " - Reward: " << reward << std::endl;
        }
    }

    return 0;
}
