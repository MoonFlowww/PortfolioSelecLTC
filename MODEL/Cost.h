double mse_loss(const std::vector<double>& predictions, const std::vector<double>& targets) {
    double loss = 0.0;
    for (int i = 0; i < predictions.size(); ++i) {
        loss += std::pow(predictions[i] - targets[i], 2);
    }
    return loss / predictions.size();
}

std::vector<double> mse_grad(const std::vector<double>& predictions, const std::vector<double>& targets) {
    std::vector<double> grad(predictions.size(), 0.0);
    for (int i = 0; i < predictions.size(); ++i) {
        grad[i] = 2 * (predictions[i] - targets[i]) / predictions.size();
    }
    return grad;
}
