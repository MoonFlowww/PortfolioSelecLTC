class DenseLayer {
public:
    int input_size;
    int output_size;
    std::vector<std::vector<double>> weights;
    std::vector<double> biases;

    DenseLayer(int input_size, int output_size) : input_size(input_size), output_size(output_size) {
        initialize_parameters();
    }


    std::vector<double> forward(const std::vector<double>& input) {
        std::vector<double> output(output_size, 0.0);
        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < input_size; ++j) {
                output[i] += input[j] * weights[i][j];
            }
            output[i] += biases[i];
        }
        return output;
    }

    void backward(const std::vector<double>& input, const std::vector<double>& grad_output,
        std::vector<std::vector<double>>& dW, std::vector<double>& dB) {
        dW = std::vector<std::vector<double>>(output_size, std::vector<double>(input_size, 0.0));
        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < input_size; ++j) {
                dW[i][j] = grad_output[i] * input[j];
            }
        }

        dB = grad_output;
    }

private:

    void initialize_parameters() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-0.5, 0.5);

        weights.resize(output_size, std::vector<double>(input_size));
        biases.resize(output_size);

        for (int i = 0; i < output_size; ++i) {
            for (int j = 0; j < input_size; ++j) {
                weights[i][j] = dis(gen);
            }
            biases[i] = dis(gen);
        }
    }
};
