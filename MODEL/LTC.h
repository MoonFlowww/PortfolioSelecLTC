//translate to c++ from a paper
#include <vector>
#include <random>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric>

enum class MappingType { Identity, Linear, Affine };
enum class ODESolver { SemiImplicit, Explicit, RungeKutta };


double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}


class LTCCell {
public:
    int num_units;
    int input_size;
    int ode_solver_unfolds;
    ODESolver solver;
    MappingType input_mapping;

    std::vector<std::vector<double>> W, sensory_W, sensory_erev, erev;
    std::vector<double> cm_t, gleak, vleak;

    LTCCell(int num_units, int input_size) : num_units(num_units), input_size(input_size) {
        ode_solver_unfolds = 6;
        solver = ODESolver::SemiImplicit;
        input_mapping = MappingType::Affine;
        init_parameters();
    }

    void init_parameters() {
        W = random_matrix(num_units, num_units, 0.01, 1.0);
        sensory_W = random_matrix(input_size, num_units, 0.01, 1.0);
        erev = random_matrix(num_units, num_units, -1, 1);
        sensory_erev = random_matrix(input_size, num_units, -1, 1);
        cm_t = std::vector<double>(num_units, 0.5);
        gleak = std::vector<double>(num_units, 1.0);
        vleak = std::vector<double>(num_units, 0.0);
    }

    std::vector<double> ode_step(const std::vector<double>& inputs, const std::vector<double>& state) {
        std::vector<double> v_pre = state;
        for (int t = 0; t < ode_solver_unfolds; ++t) {
            v_pre = update_state(inputs, v_pre);
        }
        return v_pre;
    }

    std::vector<double> update_state(const std::vector<double>& inputs, const std::vector<double>& state) {
        std::vector<double> new_state(num_units);
        for (int i = 0; i < num_units; ++i) {
            double weighted_sum = 0.0;
            for (int j = 0; j < num_units; ++j) {
                weighted_sum += W[i][j] * sigmoid(state[j]);
            }
            new_state[i] = (cm_t[i] * state[i] + gleak[i] * vleak[i] + weighted_sum) / (cm_t[i] + gleak[i]);
        }
        return new_state;
    }

    std::vector<double> operator()(const std::vector<double>& inputs, const std::vector<double>& state) {
        return ode_step(inputs, state);
    }

private:
    std::vector<std::vector<double>> random_matrix(int rows, int cols, double min_val, double max_val) {
        std::vector<std::vector<double>> mat(rows, std::vector<double>(cols));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min_val, max_val);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                mat[i][j] = dis(gen);
            }
        }
        return mat;
    }
};
