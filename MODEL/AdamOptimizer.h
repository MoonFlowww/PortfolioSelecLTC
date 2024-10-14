#include <vector>
#include <cmath>
#include <algorithm>

class AdamOptimizer {
public:
    double learning_rate;
    double beta1;
    double beta2;
    double epsilon;

    std::vector<std::vector<double>> m_weights, v_weights;
    std::vector<double> m_biases, v_biases;

    AdamOptimizer(double lr, double b1, double b2, double eps) : learning_rate(lr), beta1(b1), beta2(b2), epsilon(eps) {}


    void initialize(const std::vector<std::vector<double>>& weights, const std::vector<double>& biases) {
        m_weights = std::vector<std::vector<double>>(weights.size(), std::vector<double>(weights[0].size(), 0.0));
        v_weights = std::vector<std::vector<double>>(weights.size(), std::vector<double>(weights[0].size(), 0.0));
        m_biases = std::vector<double>(biases.size(), 0.0);
        v_biases = std::vector<double>(biases.size(), 0.0);
    }


    void update(std::vector<std::vector<double>>& weights, std::vector<double>& biases,
        const std::vector<std::vector<double>>& dW, const std::vector<double>& dB, int t) {

        double lr_t = learning_rate * std::sqrt(1 - std::pow(beta2, t)) / (1 - std::pow(beta1, t));

        for (int i = 0; i < weights.size(); ++i) {
            for (int j = 0; j < weights[0].size(); ++j) {
                m_weights[i][j] = beta1 * m_weights[i][j] + (1 - beta1) * dW[i][j];
                v_weights[i][j] = beta2 * v_weights[i][j] + (1 - beta2) * std::pow(dW[i][j], 2);


                weights[i][j] -= lr_t * m_weights[i][j] / (std::sqrt(v_weights[i][j]) + epsilon);
            }
        }

        for (int i = 0; i < biases.size(); ++i) {
            m_biases[i] = beta1 * m_biases[i] + (1 - beta1) * dB[i];
            v_biases[i] = beta2 * v_biases[i] + (1 - beta2) * std::pow(dB[i], 2);

            biases[i] -= lr_t * m_biases[i] / (std::sqrt(v_biases[i]) + epsilon);
        }
    }
};
