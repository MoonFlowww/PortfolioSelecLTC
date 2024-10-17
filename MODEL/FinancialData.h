// FinancialData.h
#pragma once
#include <string>

struct FinancialData {
    std::string date;
    std::string symbol;
    double stockPrice;
    double nextMonthStockPrice;
    double interestRate;
    double unemploymentRate;
    double inflation;
    double growthRate;
    double consumerSentiment;
    double sectorSentiment;
    double salesFigures;
    double grossMargin;
    double selfFinancingCapacity;
    double netIncome;
    double profitPerStock;
    double freeCashFlow;
    double netDebtToEquity;
    double roa;
    double ebitda;
    double pricingDCF;
    double sharpeRatio;
    double cagr;
    double var;
    double cvar;
    double beta;
    double dividendYield;
};
