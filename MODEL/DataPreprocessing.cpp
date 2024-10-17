#include "DataPreprocessing.h"
#include <algorithm>
#include <cmath>
#include <functional>

void normalizeData(std::vector<FinancialData>& data) {

    typedef double FinancialData::* MemberPtr;
    std::vector<std::pair<std::string, MemberPtr>> attributes = {
        {"stockPrice", &FinancialData::stockPrice},
        {"nextMonthStockPrice", &FinancialData::nextMonthStockPrice},
        {"interestRate", &FinancialData::interestRate},
        {"unemploymentRate", &FinancialData::unemploymentRate},
        {"inflation", &FinancialData::inflation},
        {"growthRate", &FinancialData::growthRate},
        {"consumerSentiment", &FinancialData::consumerSentiment},
        {"sectorSentiment", &FinancialData::sectorSentiment},
        {"salesFigures", &FinancialData::salesFigures},
        {"grossMargin", &FinancialData::grossMargin},
        {"selfFinancingCapacity", &FinancialData::selfFinancingCapacity},
        {"netIncome", &FinancialData::netIncome},
        {"profitPerStock", &FinancialData::profitPerStock},
        {"freeCashFlow", &FinancialData::freeCashFlow},
        {"netDebtToEquity", &FinancialData::netDebtToEquity},
        {"roa", &FinancialData::roa},
        {"ebitda", &FinancialData::ebitda},
        {"pricingDCF", &FinancialData::pricingDCF},
        {"sharpeRatio", &FinancialData::sharpeRatio},
        {"cagr", &FinancialData::cagr},
        {"var", &FinancialData::var},
        {"cvar", &FinancialData::cvar},
        {"beta", &FinancialData::beta},
        {"dividendYield", &FinancialData::dividendYield}
    };

    for (size_t i = 0; i < attributes.size(); ++i) {
        std::string& name = attributes[i].first;
        MemberPtr memberPtr = attributes[i].second;

        // avg
        double sum = 0.0;
        for (const auto& fd : data) {
            sum += fd.*memberPtr;
        }
        double mean = sum / data.size();

        // std
        double variance = 0.0;
        for (const auto& fd : data) {
            double val = fd.*memberPtr;
            variance += (val - mean) * (val - mean);
        }
        double stddev = std::sqrt(variance / data.size());

        // norm
        for (auto& fd : data) {
            double& val = fd.*memberPtr;
            if (stddev != 0)
                val = (val - mean) / stddev;
            else
                val = 0.0;
        }
    }
}
