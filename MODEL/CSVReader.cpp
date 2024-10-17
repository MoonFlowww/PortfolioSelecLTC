#include "CSVReader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

std::vector<FinancialData> loadFinancialData(const std::string& filename) {
    std::vector<FinancialData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erreur lors de l'ouverture du fichier CSV." << std::endl;
        return data;
    }

    std::string line;

    std::getline(file, line);


    std::map<std::string, std::vector<FinancialData>> dataBySymbol;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        FinancialData fd;
        std::string token;

        auto readValue = [&](double& field) {
            if (std::getline(ss, token, ',')) {
                if (token.empty() || token == "NA") {
                    field = 0.0; 
                }
                else {
                    field = std::stod(token);
                }
            }
            };


        std::getline(ss, fd.date, ',');


        std::getline(ss, fd.symbol, ',');

        size_t underscore_pos = fd.symbol.find('_');
        if (underscore_pos != std::string::npos) {

            fd.symbol = fd.symbol.substr(underscore_pos + 1);
        }

        readValue(fd.stockPrice);
        fd.nextMonthStockPrice = 0.0; 
        readValue(fd.interestRate);
        readValue(fd.unemploymentRate);
        readValue(fd.inflation);
        readValue(fd.growthRate);
        readValue(fd.consumerSentiment);
        readValue(fd.sectorSentiment);
        readValue(fd.salesFigures);
        readValue(fd.grossMargin);
        readValue(fd.selfFinancingCapacity);
        readValue(fd.netIncome);
        readValue(fd.profitPerStock);
        readValue(fd.freeCashFlow);
        readValue(fd.netDebtToEquity);
        readValue(fd.roa);
        readValue(fd.ebitda);
        readValue(fd.pricingDCF);
        readValue(fd.sharpeRatio);
        readValue(fd.cagr);
        readValue(fd.var);
        readValue(fd.cvar);
        readValue(fd.beta);
        readValue(fd.dividendYield);

        dataBySymbol[fd.symbol].push_back(fd);
    }

    file.close();

    for (auto& pair : dataBySymbol) {
        const std::string& symbol = pair.first;
        std::vector<FinancialData>& records = pair.second;
        std::sort(records.begin(), records.end(), [](const FinancialData& a, const FinancialData& b) {
            return a.date < b.date;
            });


        for (size_t i = 0; i < records.size(); ++i) {
            if (i < records.size() - 1) {
                records[i].nextMonthStockPrice = records[i + 1].stockPrice;
            }
            else {

                records[i].nextMonthStockPrice = records[i].stockPrice; 
            }
            data.push_back(records[i]);
        }
    }

    return data;
}
