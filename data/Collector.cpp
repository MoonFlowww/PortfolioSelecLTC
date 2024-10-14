#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <map>
#include <algorithm>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string httpGet(const std::string& url)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

std::vector<std::string> getLast12Months()
{
    std::vector<std::string> dates;
    time_t t = time(0);
    tm* now = localtime(&t);

    for(int i = 0; i < 12; ++i)
    {
        tm temp = *now;
        temp.tm_mon -= i;
        mktime(&temp);
        char buffer[8];
        strftime(buffer, sizeof(buffer), "%Y%m", &temp);
        dates.push_back(std::string(buffer));
    }

    std::reverse(dates.begin(), dates.end());
    return dates;
}

int main()
{
    const std::string alphaVantageApiKey = "YOUR_ALPHA_VANTAGE_API_KEY";
    const std::string fredApiKey = "YOUR_FRED_API_KEY";

    // 100
    std::vector<std::string> assets = {
        "AAPL", "MSFT", "GOOGL", "AMZN", "FB", "TSLA", "BRK.B", "JNJ", "V", "WMT",
        "JPM", "UNH", "NVDA", "HD", "PG", "DIS", "MA", "BAC", "PYPL", "VZ",
        "ADBE", "CMCSA", "NFLX", "INTC", "T", "CRM", "PFE", "KO", "TMO", "CSCO",
        "PEP", "ABT", "AVGO", "NKE", "XOM", "ORCL", "COST", "LLY", "QCOM", "TXN",
        "MRK", "MCD", "NEE", "MDT", "HON", "AMGN", "WFC", "LOW", "UPS", "IBM",
        "INTU", "LIN", "UNP", "SBUX", "MS", "MMM", "RTX", "BLK", "GS", "AMD",
        "PLD", "GILD", "NOW", "ISRG", "BA", "BKNG", "AXP", "CVX", "DE", "ZTS",
        "CAT", "ADP", "LMT", "CI", "MO", "AMAT", "BMY", "ADI", "SCHW", "COP",
        "CCI", "CB", "PNC", "SYK", "USB", "FDX", "APD", "FIS", "MMC", "ICE",
        "CL", "EL", "SO", "CSX", "TJX", "D", "DUK", "BDX", "SPGI", "EW"
    };

    std::vector<std::string> last12Months = getLast12Months();

    std::ofstream csvFile("financial_data.csv");
    if (!csvFile.is_open())
    {
        std::cerr << "Failed to open CSV file." << std::endl;
        return 1;
    }

    csvFile << "Date,Symbol,Stock Price,Interest Rate,Unemployment Rate,Inflation,Growth Rate,Consumer Sentiment,Sector Sentiment,"
            << "Sales Figures,Gross Margin,Self Financing Capacity,Net Income,Profit Per Stock,Free Cash Flow,"
            << "Net Debt to Equity,ROA,EBITDA,Pricing DCF,Sharpe Ratio,CAGR,VaR,CVaR,Beta,Dividend Yield\n";

    std::map<std::string, double> interestRates;
    std::map<std::string, double> unemploymentRates;
    std::map<std::string, double> inflations;
    std::map<std::string, double> growthRates;
    std::map<std::string, double> consumerSentiments;

    for (const auto& month : last12Months)
    {
        std::string startDate = month + "01";
        std::string endDate = month + "31";

        // Interest Rate
        {
            std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=DFF&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            std::string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double rate = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                    interestRates[month] = rate;
                }
                else
                {
                    interestRates[month] = interestRates.empty() ? 0.0 : interestRates.rbegin()->second;
                }
            }
        }

        // Unemployment Rate
        {
            std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UNRATE&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            std::string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double rate = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                    unemploymentRates[month] = rate;
                }
                else
                {
                    unemploymentRates[month] = unemploymentRates.empty() ? 0.0 : unemploymentRates.rbegin()->second;
                }
            }
        }

        // Inflation Rate (CPI YoY)
        {
            std::string cpiUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=CPIAUCSL&api_key=" + fredApiKey + "&file_type=json&observation_start=";
            std::string currentMonthStart = month + "01";
            std::string currentMonthEnd = month + "31";

            tm temp;
            strptime(month.c_str(), "%Y%m", &temp);
            temp.tm_year -= 1;
            mktime(&temp);
            char buffer[8];
            strftime(buffer, sizeof(buffer), "%Y%m", &temp);
            std::string lastYearMonth = std::string(buffer);
            std::string lastYearStart = lastYearMonth + "01";
            std::string lastYearEnd = lastYearMonth + "31";

            double currentCPI = 0.0;
            std::string data = httpGet(cpiUrl + currentMonthStart + "&observation_end=" + currentMonthEnd);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    currentCPI = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                }
            }

            double lastYearCPI = 0.0;
            data = httpGet(cpiUrl + lastYearStart + "&observation_end=" + lastYearEnd);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    lastYearCPI = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                }
            }

            if (currentCPI != 0 && lastYearCPI != 0)
                inflations[month] = ((currentCPI - lastYearCPI) / lastYearCPI) * 100.0;
            else
                inflations[month] = inflations.empty() ? 0.0 : inflations.rbegin()->second;
        }

        // GDP Growth Rate (Quarterly data)
        {
            if (growthRates.empty())
            {
                std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=A191RL1Q225SBEA&api_key=" + fredApiKey + "&file_type=json";
                std::string data = httpGet(fredUrl);
                if (!data.empty())
                {
                    auto jsonData = nlohmann::json::parse(data);
                    if (!jsonData["observations"].empty())
                    {
                        double rate = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                        for (const auto& m : last12Months)
                            growthRates[m] = rate;
                    }
                }
            }
        }

        // Consumer Sentiment
        {
            std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UMCSENT&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            std::string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double value = std::stod(jsonData["observations"].back()["value"].get<std::string>());
                    consumerSentiments[month] = value;
                }
                else
                {
                    consumerSentiments[month] = consumerSentiments.empty() ? 0.0 : consumerSentiments.rbegin()->second;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Sector Sentiment using sector ETF (e.g., XLK for Technology)
    std::map<std::string, double> sectorSentiments;
    std::string sectorSymbol = "XLK";
    std::map<std::string, double> sectorPrices;
    {
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + sectorSymbol + "&apikey=" + alphaVantageApiKey;
        std::string data = httpGet(url);
        if (!data.empty())
        {
            auto jsonData = nlohmann::json::parse(data);
            if (jsonData.contains("Monthly Adjusted Time Series"))
            {
                auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                for (const auto& month : last12Months)
                {
                    std::string dateStr = month.substr(0,4) + "-" + month.substr(4,2) + "-28";
                    if (timeSeries.contains(dateStr))
                    {
                        double adjustedClose = std::stod(timeSeries[dateStr]["5. adjusted close"].get<std::string>());
                        sectorPrices[month] = adjustedClose;
                    }
                }
            }
        }
    }

    for (size_t i = 1; i < last12Months.size(); ++i)
    {
        const std::string& currentMonth = last12Months[i];
        const std::string& prevMonth = last12Months[i - 1];
        if (sectorPrices.count(currentMonth) && sectorPrices.count(prevMonth))
        {
            double currentPrice = sectorPrices[currentMonth];
            double prevPrice = sectorPrices[prevMonth];
            double change = ((currentPrice - prevPrice) / prevPrice) * 100.0;
            sectorSentiments[currentMonth] = change;
        }
        else
        {
            sectorSentiments[currentMonth] = 0.0;
        }
    }
    sectorSentiments[last12Months[0]] = 0.0;

    for(const auto& symbol : assets)
    {
        std::cout << "Processing: " << symbol << std::endl;

        std::map<std::string, double> stockPrices;
        {
            std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (jsonData.contains("Monthly Adjusted Time Series"))
                {
                    auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                    for (const auto& month : last12Months)
                    {
                        std::string dateStr = month.substr(0,4) + "-" + month.substr(4,2) + "-28";
                        if (timeSeries.contains(dateStr))
                        {
                            double adjustedClose = std::stod(timeSeries[dateStr]["5. adjusted close"].get<std::string>());
                            stockPrices[month] = adjustedClose;
                        }
                    }
                }
            }
        }

        // Company Overview from Alpha Vantage
        double beta = 0.0;
        double dividendYield = 0.0;
        double ebitda = 0.0;
        {
            std::string url = "https://www.alphavantage.co/query?function=OVERVIEW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (!jsonData.is_null() && jsonData.contains("Symbol"))
                {
                    if (jsonData.contains("EBITDA") && !jsonData["EBITDA"].empty())
                        ebitda = std::stod(jsonData["EBITDA"].get<std::string>());

                    if (jsonData.contains("DividendYield") && !jsonData["DividendYield"].empty() && jsonData["DividendYield"] != "None")
                        dividendYield = std::stod(jsonData["DividendYield"].get<std::string>());

                    if (jsonData.contains("Beta") && !jsonData["Beta"].empty() && jsonData["Beta"] != "None")
                        beta = std::stod(jsonData["Beta"].get<std::string>());
                }
            }
        }

        // Financial Statements
        double salesFigures = 0.0;
        double grossMargin = 0.0;
        double selfFinancingCapacity = 0.0;
        double netIncome = 0.0;
        double profitPerStock = 0.0;
        double freeCashFlow = 0.0;
        double netDebtToEquity = 0.0;
        double roa = 0.0;

        double totalAssets = 0.0;
        double totalEquity = 0.0;
        double totalLiabilities = 0.0;
        double netDebt = 0.0;

        // Income Statement
        {
            std::string url = "https://www.alphavantage.co/query?function=INCOME_STATEMENT&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalRevenue"))
                        salesFigures = std::stod(report["totalRevenue"].get<std::string>());
                    if (report.contains("grossProfit"))
                        grossMargin = std::stod(report["grossProfit"].get<std::string>()) / salesFigures * 100.0;
                    if (report.contains("netIncome"))
                        netIncome = std::stod(report["netIncome"].get<std::string>());
                    if (report.contains("eps"))
                        profitPerStock = std::stod(report["eps"].get<std::string>());
                }
            }
        }

        // Balance Sheet
        {
            std::string url = "https://www.alphavantage.co/query?function=BALANCE_SHEET&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalAssets"))
                        totalAssets = std::stod(report["totalAssets"].get<std::string>());
                    if (report.contains("totalShareholderEquity"))
                        totalEquity = std::stod(report["totalShareholderEquity"].get<std::string>());
                    if (report.contains("totalLiabilities"))
                        totalLiabilities = std::stod(report["totalLiabilities"].get<std::string>());
                    if (report.contains("cashAndCashEquivalentsAtCarryingValue"))
                        netDebt = totalLiabilities - std::stod(report["cashAndCashEquivalentsAtCarryingValue"].get<std::string>());
                }
            }
        }

        // Cash Flow Statement
        {
            std::string url = "https://www.alphavantage.co/query?function=CASH_FLOW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("operatingCashflow"))
                        selfFinancingCapacity = std::stod(report["operatingCashflow"].get<std::string>());
                    if (report.contains("freeCashFlow"))
                        freeCashFlow = std::stod(report["freeCashFlow"].get<std::string>());
                    else
                    {
                        if (report.contains("operatingCashflow") && report.contains("capitalExpenditures"))
                        {
                            double operatingCashflow = std::stod(report["operatingCashflow"].get<std::string>());
                            double capitalExpenditures = std::stod(report["capitalExpenditures"].get<std::string>());
                            freeCashFlow = operatingCashflow - capitalExpenditures;
                        }
                    }
                }
            }
        }

        if (totalEquity != 0)
            netDebtToEquity = (netDebt / totalEquity) * 100.0;

        if (totalAssets != 0)
            roa = (netIncome / totalAssets) * 100.0;

        // Historical daily prices
        std::vector<double> historicalPrices;
        {
            std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY_ADJUSTED&symbol=" + symbol + "&outputsize=full&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = nlohmann::json::parse(data);
                if (jsonData.contains("Time Series (Daily)"))
                {
                    auto timeSeries = jsonData["Time Series (Daily)"];
                    for (auto it = timeSeries.begin(); it != timeSeries.end(); ++it)
                    {
                        double adjustedClose = std::stod(it.value()["5. adjusted close"].get<std::string>());
                        historicalPrices.push_back(adjustedClose);
                    }
                }
            }
        }

        // Performance Metrics
        double sharpeRatio = 0.0;
        double cagr = 0.0;
        double var = 0.0;
        double cvar = 0.0;

        if (historicalPrices.size() >= 252)
        {
            std::vector<double> returns;
            for (size_t i = 1; i < 252; ++i)
            {
                double dailyReturn = (historicalPrices[i - 1] - historicalPrices[i]) / historicalPrices[i];
                returns.push_back(dailyReturn);
            }

            double avgReturn = 0.0;
            for (double r : returns)
                avgReturn += r;
            avgReturn /= returns.size();

            double stdDev = 0.0;
            for (double r : returns)
                stdDev += pow(r - avgReturn, 2);
            stdDev = sqrt(stdDev / (returns.size() - 1));

            double annualizedReturn = avgReturn * 252;
            double annualizedStdDev = stdDev * sqrt(252);

            double riskFreeRate = interestRates[last12Months.back()] / 100.0;
            if (annualizedStdDev != 0)
                sharpeRatio = (annualizedReturn - riskFreeRate) / annualizedStdDev;

            double endingValue = historicalPrices[0];
            double beginningValue = historicalPrices[251];
            cagr = pow((endingValue / beginningValue), (1.0)) - 1;

            std::sort(returns.begin(), returns.end());
            size_t index = static_cast<size_t>(0.05 * returns.size());
            var = returns[index];

            double sumLosses = 0.0;
            for (size_t i = 0; i <= index; ++i)
                sumLosses += returns[i];
            cvar = sumLosses / (index + 1);
        }

        double pricingDCF = 0.0;

        for (const auto& month : last12Months)
        {
            double stockPrice = stockPrices.count(month) ? stockPrices[month] : 0.0;
            double interestRate = interestRates.count(month) ? interestRates[month] : 0.0;
            double unemploymentRate = unemploymentRates.count(month) ? unemploymentRates[month] : 0.0;
            double inflation = inflations.count(month) ? inflations[month] : 0.0;
            double growthRate = growthRates.count(month) ? growthRates[month] : 0.0;
            double consumerSentiment = consumerSentiments.count(month) ? consumerSentiments[month] : 0.0;
            double sectorSentiment = sectorSentiments.count(month) ? sectorSentiments[month] : 0.0;

            csvFile << month << ","
                    << symbol << ","
                    << stockPrice << ","
                    << interestRate << ","
                    << unemploymentRate << ","
                    << inflation << ","
                    << growthRate << ","
                    << consumerSentiment << ","
                    << sectorSentiment << ","
                    << salesFigures << ","
                    << grossMargin << ","
                    << selfFinancingCapacity << ","
                    << netIncome << ","
                    << profitPerStock << ","
                    << freeCashFlow << ","
                    << netDebtToEquity << ","
                    << roa << ","
                    << ebitda << ","
                    << pricingDCF << ","
                    << sharpeRatio << ","
                    << cagr << ","
                    << var << ","
                    << cvar << ","
                    << beta << ","
                    << dividendYield << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(12));
    }

    csvFile.close();
    std::cout << "Data collection complete. Check 'financial_data.csv'." << std::endl;

    return 0;
}
