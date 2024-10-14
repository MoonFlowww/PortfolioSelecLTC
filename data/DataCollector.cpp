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

using json = nlohmann::json;
using namespace std;


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


string httpGet(const string& url)
{
    CURL* curl;
    CURLcode res;
    string readBuffer;

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
            cerr << "cURL error: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

vector<string> getLast12Months()
{
    vector<string> dates;
    time_t t = time(0);
    tm* now = localtime(&t);

    for(int i = 0; i < 12; ++i)
    {
        tm temp = *now;
        temp.tm_mon -= i;
        mktime(&temp);
        char buffer[8];
        strftime(buffer, sizeof(buffer), "%Y%m", &temp);
        dates.push_back(string(buffer));
    }

    reverse(dates.begin(), dates.end());
    return dates;
}

int main()
{
    const string alphaVantageApiKey = "YOUR_ALPHA_VANTAGE_API_KEY";
    const string fredApiKey = "YOUR_FRED_API_KEY";

    vector<string> assets = {"AAPL", "MSFT", "GOOGL"};
    vector<string> last12Months = getLast12Months();

    ofstream csvFile("financial_data.csv");
    if (!csvFile.is_open())
    {
        cerr << "Failed to open CSV file." << endl;
        return 1;
    }

    csvFile << "Date,Symbol,Stock Price,Interest Rate,Unemployment Rate,Inflation,Growth Rate,Consumer Sentiment,Sector Sentiment,"
            << "Sales Figures,Gross Margin,Self Financing Capacity,Net Income,Profit Per Stock,Free Cash Flow,"
            << "Net Debt to Equity,ROA,EBITDA,Pricing DCF,Sharpe Ratio,CAGR,VaR,CVaR,Beta,Dividend Yield\n";

    map<string, double> interestRates;
    map<string, double> unemploymentRates;
    map<string, double> inflations;
    map<string, double> growthRates;
    map<string, double> consumerSentiments;

    // Fetch economic data for each month
    for (const auto& month : last12Months)
    {
        string startDate = month + "01";
        string endDate = month + "31";

        // Interest Rate
        {
            string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=DFF&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double rate = stod(jsonData["observations"].back()["value"].get<string>());
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
            string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UNRATE&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double rate = stod(jsonData["observations"].back()["value"].get<string>());
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
            string cpiUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=CPIAUCSL&api_key=" + fredApiKey + "&file_type=json&observation_start=";
            string currentMonthStart = month + "01";
            string currentMonthEnd = month + "31";

            tm temp;
            strptime(month.c_str(), "%Y%m", &temp);
            temp.tm_year -= 1;
            mktime(&temp);
            char buffer[8];
            strftime(buffer, sizeof(buffer), "%Y%m", &temp);
            string lastYearMonth = string(buffer);
            string lastYearStart = lastYearMonth + "01";
            string lastYearEnd = lastYearMonth + "31";

            double currentCPI = 0.0;
            string data = httpGet(cpiUrl + currentMonthStart + "&observation_end=" + currentMonthEnd);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    currentCPI = stod(jsonData["observations"].back()["value"].get<string>());
                }
            }

            double lastYearCPI = 0.0;
            data = httpGet(cpiUrl + lastYearStart + "&observation_end=" + lastYearEnd);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    lastYearCPI = stod(jsonData["observations"].back()["value"].get<string>());
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
                string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=A191RL1Q225SBEA&api_key=" + fredApiKey + "&file_type=json";
                string data = httpGet(fredUrl);
                if (!data.empty())
                {
                    auto jsonData = json::parse(data);
                    if (!jsonData["observations"].empty())
                    {
                        double rate = stod(jsonData["observations"].back()["value"].get<string>());
                        for (const auto& m : last12Months)
                            growthRates[m] = rate;
                    }
                }
            }
        }

        // Consumer Sentiment
        {
            string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UMCSENT&api_key=" + fredApiKey + "&file_type=json&observation_start=" + startDate + "&observation_end=" + endDate;
            string data = httpGet(fredUrl);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData["observations"].empty())
                {
                    double value = stod(jsonData["observations"].back()["value"].get<string>());
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
    map<string, double> sectorSentiments;
    string sectorSymbol = "XLK";
    map<string, double> sectorPrices;
    {
        string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + sectorSymbol + "&apikey=" + alphaVantageApiKey;
        string data = httpGet(url);
        if (!data.empty())
        {
            auto jsonData = json::parse(data);
            if (jsonData.contains("Monthly Adjusted Time Series"))
            {
                auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                for (const auto& month : last12Months)
                {
                    string dateStr = month.substr(0,4) + "-" + month.substr(4,2) + "-28";
                    if (timeSeries.contains(dateStr))
                    {
                        double adjustedClose = stod(timeSeries[dateStr]["5. adjusted close"].get<string>());
                        sectorPrices[month] = adjustedClose;
                    }
                }
            }
        }
    }

    for (size_t i = 1; i < last12Months.size(); ++i)
    {
        const string& currentMonth = last12Months[i];
        const string& prevMonth = last12Months[i - 1];
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
        cout << "Processing: " << symbol << endl;

        map<string, double> stockPrices;
        {
            string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (jsonData.contains("Monthly Adjusted Time Series"))
                {
                    auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                    for (const auto& month : last12Months)
                    {
                        string dateStr = month.substr(0,4) + "-" + month.substr(4,2) + "-28";
                        if (timeSeries.contains(dateStr))
                        {
                            double adjustedClose = stod(timeSeries[dateStr]["5. adjusted close"].get<string>());
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
            string url = "https://www.alphavantage.co/query?function=OVERVIEW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (!jsonData.is_null() && jsonData.contains("Symbol"))
                {
                    if (jsonData.contains("EBITDA") && !jsonData["EBITDA"].empty())
                        ebitda = stod(jsonData["EBITDA"].get<string>());

                    if (jsonData.contains("DividendYield") && !jsonData["DividendYield"].empty() && jsonData["DividendYield"] != "None")
                        dividendYield = stod(jsonData["DividendYield"].get<string>());

                    if (jsonData.contains("Beta") && !jsonData["Beta"].empty() && jsonData["Beta"] != "None")
                        beta = stod(jsonData["Beta"].get<string>());
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
            string url = "https://www.alphavantage.co/query?function=INCOME_STATEMENT&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalRevenue"))
                        salesFigures = stod(report["totalRevenue"].get<string>());
                    if (report.contains("grossProfit"))
                        grossMargin = stod(report["grossProfit"].get<string>()) / salesFigures * 100.0;
                    if (report.contains("netIncome"))
                        netIncome = stod(report["netIncome"].get<string>());
                    if (report.contains("eps"))
                        profitPerStock = stod(report["eps"].get<string>());
                }
            }
        }

        // Balance Sheet
        {
            string url = "https://www.alphavantage.co/query?function=BALANCE_SHEET&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalAssets"))
                        totalAssets = stod(report["totalAssets"].get<string>());
                    if (report.contains("totalShareholderEquity"))
                        totalEquity = stod(report["totalShareholderEquity"].get<string>());
                    if (report.contains("totalLiabilities"))
                        totalLiabilities = stod(report["totalLiabilities"].get<string>());
                    if (report.contains("cashAndCashEquivalentsAtCarryingValue"))
                        netDebt = totalLiabilities - stod(report["cashAndCashEquivalentsAtCarryingValue"].get<string>());
                }
            }
        }

        // Cash Flow Statement
        {
            string url = "https://www.alphavantage.co/query?function=CASH_FLOW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("operatingCashflow"))
                        selfFinancingCapacity = stod(report["operatingCashflow"].get<string>());
                    if (report.contains("freeCashFlow"))
                        freeCashFlow = stod(report["freeCashFlow"].get<string>());
                    else
                    {
                        if (report.contains("operatingCashflow") && report.contains("capitalExpenditures"))
                        {
                            double operatingCashflow = stod(report["operatingCashflow"].get<string>());
                            double capitalExpenditures = stod(report["capitalExpenditures"].get<string>());
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
        vector<double> historicalPrices;
        {
            string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY_ADJUSTED&symbol=" + symbol + "&outputsize=full&apikey=" + alphaVantageApiKey;
            string data = httpGet(url);
            if (!data.empty())
            {
                auto jsonData = json::parse(data);
                if (jsonData.contains("Time Series (Daily)"))
                {
                    auto timeSeries = jsonData["Time Series (Daily)"];
                    for (auto it = timeSeries.begin(); it != timeSeries.end(); ++it)
                    {
                        double adjustedClose = stod(it.value()["5. adjusted close"].get<string>());
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
            vector<double> returns;
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

            sort(returns.begin(), returns.end());
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
    cout << "Data collection complete. Check 'financial_data.csv'." << endl;

    return 0;
}
