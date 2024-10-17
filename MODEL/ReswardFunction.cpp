#include "RewardFunction.h"
#include <cmath>

double compute_reward(const FinancialData& fd) {
    if (fd.stockPrice != 0.0 && fd.nextMonthStockPrice != 0.0) {
        double percentageChange = (fd.nextMonthStockPrice - fd.stockPrice) / fd.stockPrice;
        double reward = std::pow(percentageChange, 3); // exponentiel component ^3
        return reward;
    }
    else { //no data -> 0
        return 0.0;
    }
}
