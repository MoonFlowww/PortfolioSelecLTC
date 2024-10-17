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
#include <numeric>
#include <sstream>
#include <cstdio>

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
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Set a user agent to avoid being blocked by some servers
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

// Function to get list of months covering the last 'years' years
std::vector<std::string> getMonthsSince(int years)
{
    std::vector<std::string> months;

    std::time_t t = std::time(nullptr);
    std::tm now = *std::localtime(&t);

    // Calculate the start date (years ago)
    std::tm startDate = now;
    startDate.tm_year -= years;
    mktime(&startDate);

    // Loop from startDate to now, adding each month to the list
    std::tm current = startDate;
    while ((current.tm_year < now.tm_year) || (current.tm_year == now.tm_year && current.tm_mon <= now.tm_mon))
    {
        char buffer[7];
        std::strftime(buffer, sizeof(buffer), "%Y%m", &current);
        months.push_back(buffer);

        // Move to next month
        current.tm_mon += 1;
        if (current.tm_mon > 11)
        {
            current.tm_mon = 0;
            current.tm_year += 1;
        }
        mktime(&current);
    }

    return months;
}

int main()
{
    const std::string alphaVantageApiKey = "YOUR_ALPHA_VANTAGE_API_KEY";
    const std::string fredApiKey = "YOUR_FRED_API_KEY";

    // List of assets
     std::vector<std::string> assets = {
        "AAPL", "MSFT", "GOOGL", "AMZN", "FB", "TSLA", "JNJ", "V", "WMT",
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

    std::ofstream csvFile("financial_data.csv");
    if (!csvFile.is_open())
    {
        std::cerr << "Failed to open CSV file." << std::endl;
        return 1;
    }

    csvFile << "Date,Symbol,Stock Price,Interest Rate,Unemployment Rate,Inflation,Growth Rate,Consumer Sentiment,Sector Sentiment,"
        << "Sales Figures,Gross Margin,Self Financing Capacity,Net Income,Profit Per Stock,Free Cash Flow,"
        << "Net Debt to Equity,ROA,EBITDA,Pricing DCF,Sharpe Ratio,CAGR,VaR,CVaR,Beta,Dividend Yield\n";

    // Generate list of months covering the last 5 years
    int years = 5;
    std::vector<std::string> lastMonths = getMonthsSince(years);

    // Calculate earliest date string in format "YYYY-MM-DD"
    std::time_t currentTime = std::time(nullptr);
    std::tm earliestDateTm;
    localtime_s(&earliestDateTm, &currentTime);

    earliestDateTm.tm_year -= years;
    earliestDateTm.tm_mon = 0; // January
    earliestDateTm.tm_mday = 1;
    mktime(&earliestDateTm);

    char earliestDateBuffer[11];
    std::strftime(earliestDateBuffer, sizeof(earliestDateBuffer), "%Y-%m-%d", &earliestDateTm);
    std::string earliestDate = earliestDateBuffer;

    // Fetch Economic Indicators Once
    std::map<std::string, double> interestRates;
    std::map<std::string, double> unemploymentRates;
    std::map<std::string, double> inflations;
    std::map<std::string, double> growthRates;
    std::map<std::string, double> consumerSentiments;

    // Fetch Interest Rate data
    {
        std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=DFF&api_key=" + fredApiKey + "&file_type=json&frequency=m&aggregation_method=avg&observation_start=" + earliestDate;
        std::string data = httpGet(fredUrl);
        if (data.empty())
        {
            std::cerr << "Failed to fetch Interest Rate data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse Interest Rate JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("error_message"))
            {
                std::cerr << "FRED API Error (Interest Rate): " << jsonData["error_message"].get<std::string>() << std::endl;
            }
            else
            {
                for (auto& obs : jsonData["observations"])
                {
                    std::string date = obs["date"].get<std::string>(); // Format: YYYY-MM-DD
                    std::string value = obs["value"].get<std::string>();
                    if (value != ".")
                    {
                        std::string month = date.substr(0, 4) + date.substr(5, 2);
                        if (std::find(lastMonths.begin(), lastMonths.end(), month) != lastMonths.end())
                        {
                            interestRates[month] = std::stod(value);
                        }
                    }
                }
            }
        }
    }

    // Fetch Unemployment Rate data
    {
        std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UNRATE&api_key=" + fredApiKey + "&file_type=json&frequency=m&aggregation_method=avg&observation_start=" + earliestDate;
        std::string data = httpGet(fredUrl);
        if (data.empty())
        {
            std::cerr << "Failed to fetch Unemployment Rate data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse Unemployment Rate JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("error_message"))
            {
                std::cerr << "FRED API Error (Unemployment Rate): " << jsonData["error_message"].get<std::string>() << std::endl;
            }
            else
            {
                for (auto& obs : jsonData["observations"])
                {
                    std::string date = obs["date"].get<std::string>(); // Format: YYYY-MM-DD
                    std::string value = obs["value"].get<std::string>();
                    if (value != ".")
                    {
                        std::string month = date.substr(0, 4) + date.substr(5, 2);
                        if (std::find(lastMonths.begin(), lastMonths.end(), month) != lastMonths.end())
                        {
                            unemploymentRates[month] = std::stod(value);
                        }
                    }
                }
            }
        }
    }

    // Fetch Inflation Rate data (CPI YoY)
    {
        // Fetch CPI data
        std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=CPIAUCSL&api_key=" + fredApiKey + "&file_type=json&frequency=m&aggregation_method=avg&observation_start=" + earliestDate;
        std::string data = httpGet(fredUrl);
        std::map<std::string, double> cpiData;
        if (data.empty())
        {
            std::cerr << "Failed to fetch CPI data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse CPI JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("error_message"))
            {
                std::cerr << "FRED API Error (CPI): " << jsonData["error_message"].get<std::string>() << std::endl;
            }
            else
            {
                for (auto& obs : jsonData["observations"])
                {
                    std::string date = obs["date"].get<std::string>();
                    std::string value = obs["value"].get<std::string>();
                    if (value != ".")
                    {
                        std::string month = date.substr(0, 4) + date.substr(5, 2);
                        cpiData[month] = std::stod(value);
                    }
                }

                // Calculate YoY Inflation
                for (const auto& month : lastMonths)
                {
                    int year = std::stoi(month.substr(0, 4));
                    int mon = std::stoi(month.substr(4, 2));

                    int lastYear = year - 1;
                    char lastYearBuffer[7];
                    snprintf(lastYearBuffer, sizeof(lastYearBuffer), "%04d%02d", lastYear, mon);
                    std::string lastYearMonth = lastYearBuffer;

                    if (cpiData.count(month) && cpiData.count(lastYearMonth))
                    {
                        double currentCPI = cpiData[month];
                        double lastYearCPI = cpiData[lastYearMonth];
                        inflations[month] = ((currentCPI - lastYearCPI) / lastYearCPI) * 100.0;
                    }
                }
            }
        }
    }

    // Fetch GDP Growth Rate data (quarterly)
    {
        std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=A191RL1Q225SBEA&api_key=" + fredApiKey + "&file_type=json&frequency=q&observation_start=" + earliestDate;
        std::string data = httpGet(fredUrl);
        if (data.empty())
        {
            std::cerr << "Failed to fetch GDP Growth Rate data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse GDP Growth Rate JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("error_message"))
            {
                std::cerr << "FRED API Error (GDP Growth Rate): " << jsonData["error_message"].get<std::string>() << std::endl;
            }
            else
            {
                std::map<std::string, double> gdpData;
                for (auto& obs : jsonData["observations"])
                {
                    std::string date = obs["date"].get<std::string>();
                    std::string value = obs["value"].get<std::string>();
                    if (value != ".")
                    {
                        std::string quarter = date.substr(0, 7);
                        gdpData[quarter] = std::stod(value);
                    }
                }
                // Map quarterly data to months in lastMonths
                for (const auto& month : lastMonths)
                {
                    std::string year = month.substr(0, 4);
                    std::string mon = month.substr(4, 2);
                    int monInt = std::stoi(mon);
                    std::string quarter;
                    if (monInt <= 3)
                        quarter = year + "-01";
                    else if (monInt <= 6)
                        quarter = year + "-04";
                    else if (monInt <= 9)
                        quarter = year + "-07";
                    else
                        quarter = year + "-10";
                    if (gdpData.count(quarter))
                    {
                        growthRates[month] = gdpData[quarter];
                    }
                }
            }
        }
    }

    // Fetch Consumer Sentiment data
    {
        std::string fredUrl = "https://api.stlouisfed.org/fred/series/observations?series_id=UMCSENT&api_key=" + fredApiKey + "&file_type=json&frequency=m&aggregation_method=avg&observation_start=" + earliestDate;
        std::string data = httpGet(fredUrl);
        if (data.empty())
        {
            std::cerr << "Failed to fetch Consumer Sentiment data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse Consumer Sentiment JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("error_message"))
            {
                std::cerr << "FRED API Error (Consumer Sentiment): " << jsonData["error_message"].get<std::string>() << std::endl;
            }
            else
            {
                for (auto& obs : jsonData["observations"])
                {
                    std::string date = obs["date"].get<std::string>();
                    std::string value = obs["value"].get<std::string>();
                    if (value != ".")
                    {
                        std::string month = date.substr(0, 4) + date.substr(5, 2);
                        if (std::find(lastMonths.begin(), lastMonths.end(), month) != lastMonths.end())
                        {
                            consumerSentiments[month] = std::stod(value);
                        }
                    }
                }
            }
        }
    }

    // Fetch Sector Sentiment data once
    std::map<std::string, double> sectorSentiments;
    {
        std::string sectorSymbol = "XLK"; // Technology Select Sector SPDR Fund
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + sectorSymbol + "&apikey=" + alphaVantageApiKey;
        std::string data = httpGet(url);
        if (data.empty())
        {
            std::cerr << "Failed to fetch Sector Sentiment data." << std::endl;
        }
        else
        {
            auto jsonData = nlohmann::json::parse(data, nullptr, false);
            if (jsonData.is_discarded())
            {
                std::cerr << "Failed to parse Sector Sentiment JSON data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
            else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
            {
                std::cerr << "Alpha Vantage API Error (Sector Sentiment): " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
            }
            else if (jsonData.contains("Monthly Adjusted Time Series"))
            {
                auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                std::map<std::string, double> sectorPrices;
                std::vector<std::string> months;
                for (auto& item : timeSeries.items())
                {
                    std::string dateStr = item.key(); // format: YYYY-MM-DD
                    std::string month = dateStr.substr(0, 4) + dateStr.substr(5, 2);
                    if (std::find(lastMonths.begin(), lastMonths.end(), month) != lastMonths.end())
                    {
                        double adjustedClose = std::stod(item.value()["5. adjusted close"].get<std::string>());
                        sectorPrices[month] = adjustedClose;
                        months.push_back(month);
                    }
                }
                std::sort(months.begin(), months.end());
                for (size_t i = 1; i < months.size(); ++i)
                {
                    const std::string& currentMonth = months[i];
                    const std::string& prevMonth = months[i - 1];
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
                sectorSentiments[months[0]] = 0.0;
            }
            else
            {
                std::cerr << "Unexpected JSON structure in Sector Sentiment data." << std::endl;
                std::cerr << "Data: " << data << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }

    for (const auto& symbol : assets)
    {
        std::cout << "Processing: " << symbol << std::endl;

        std::map<std::string, double> stockPrices;
        std::vector<std::string> availableMonths;
        {
            std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY_ADJUSTED&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch stock prices for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse stock price JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Stock Prices) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else if (jsonData.contains("Monthly Adjusted Time Series"))
                {
                    auto timeSeries = jsonData["Monthly Adjusted Time Series"];
                    for (auto& item : timeSeries.items())
                    {
                        std::string dateStr = item.key(); // format: YYYY-MM-DD
                        std::string month = dateStr.substr(0, 4) + dateStr.substr(5, 2);
                        if (std::find(lastMonths.begin(), lastMonths.end(), month) != lastMonths.end())
                        {
                            double adjustedClose = std::stod(item.value()["5. adjusted close"].get<std::string>());
                            stockPrices[month] = adjustedClose;
                            availableMonths.push_back(month);
                        }
                    }
                }
                else
                {
                    std::cerr << "Unexpected JSON structure in stock price data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(15));

        }

        // Fetch Company Overview
        double beta = 0.0;
        double dividendYield = 0.0;
        double ebitda = 0.0;
        {
            std::string url = "https://www.alphavantage.co/query?function=OVERVIEW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch Company Overview for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse Company Overview JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Company Overview) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else
                {
                    if (jsonData.contains("EBITDA") && !jsonData["EBITDA"].empty())
                        ebitda = std::stod(jsonData["EBITDA"].get<std::string>());

                    if (jsonData.contains("DividendYield") && !jsonData["DividendYield"].empty() && jsonData["DividendYield"] != "None")
                        dividendYield = std::stod(jsonData["DividendYield"].get<std::string>());

                    if (jsonData.contains("Beta") && !jsonData["Beta"].empty() && jsonData["Beta"] != "None")
                        beta = std::stod(jsonData["Beta"].get<std::string>());
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }

        // Fetch Financial Statements (latest quarterly data)
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

        // Fetch Income Statement
        {
            std::string url = "https://www.alphavantage.co/query?function=INCOME_STATEMENT&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch Income Statement for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse Income Statement JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Income Statement) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalRevenue") && !report["totalRevenue"].empty())
                        salesFigures = std::stod(report["totalRevenue"].get<std::string>());
                    if (report.contains("grossProfit") && !report["grossProfit"].empty())
                    {
                        double grossProfit = std::stod(report["grossProfit"].get<std::string>());
                        grossMargin = (grossProfit / salesFigures) * 100.0;
                    }
                    if (report.contains("netIncome") && !report["netIncome"].empty())
                        netIncome = std::stod(report["netIncome"].get<std::string>());
                    if (report.contains("eps") && !report["eps"].empty())
                        profitPerStock = std::stod(report["eps"].get<std::string>());
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }

        // Balance Sheet
        {
            std::string url = "https://www.alphavantage.co/query?function=BALANCE_SHEET&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch Balance Sheet for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse Balance Sheet JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Balance Sheet) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("totalAssets") && !report["totalAssets"].empty())
                        totalAssets = std::stod(report["totalAssets"].get<std::string>());
                    if (report.contains("totalShareholderEquity") && !report["totalShareholderEquity"].empty())
                        totalEquity = std::stod(report["totalShareholderEquity"].get<std::string>());
                    if (report.contains("totalLiabilities") && !report["totalLiabilities"].empty())
                        totalLiabilities = std::stod(report["totalLiabilities"].get<std::string>());
                    if (report.contains("cashAndCashEquivalentsAtCarryingValue") && !report["cashAndCashEquivalentsAtCarryingValue"].empty())
                        netDebt = totalLiabilities - std::stod(report["cashAndCashEquivalentsAtCarryingValue"].get<std::string>());
                }
            }
            // avoid ban
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }

        // Fetch Cash Flow Statement
        {
            std::string url = "https://www.alphavantage.co/query?function=CASH_FLOW&symbol=" + symbol + "&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch Cash Flow Statement for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse Cash Flow Statement JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Cash Flow Statement) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else if (jsonData.contains("quarterlyReports") && !jsonData["quarterlyReports"].empty())
                {
                    auto report = jsonData["quarterlyReports"][0];
                    if (report.contains("operatingCashflow") && !report["operatingCashflow"].empty())
                        selfFinancingCapacity = std::stod(report["operatingCashflow"].get<std::string>());
                    if (report.contains("freeCashFlow") && !report["freeCashFlow"].empty())
                        freeCashFlow = std::stod(report["freeCashFlow"].get<std::string>());
                    else
                    {
                        if (report.contains("operatingCashflow") && report.contains("capitalExpenditures") && !report["operatingCashflow"].empty() && !report["capitalExpenditures"].empty())
                        {
                            double operatingCashflow = std::stod(report["operatingCashflow"].get<std::string>());
                            double capitalExpenditures = std::stod(report["capitalExpenditures"].get<std::string>());
                            freeCashFlow = operatingCashflow - capitalExpenditures;
                        }
                    }
                }
            }
            //respect API rate limits
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }

        if (totalEquity != 0)
            netDebtToEquity = (netDebt / totalEquity) * 100.0;

        if (totalAssets != 0)
            roa = (netIncome / totalAssets) * 100.0;


        std::vector<double> historicalPrices;
        {
            std::time_t t = std::time(nullptr);
            std::tm now;
            localtime_s(&now, &t);
            std::tm startDateTm = now;

            startDateTm.tm_year -= years;
            mktime(&startDateTm);

            char startDateBuffer[11];
            std::strftime(startDateBuffer, sizeof(startDateBuffer), "%Y-%m-%d", &startDateTm);
            std::string startDate = startDateBuffer;


            std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY_ADJUSTED&symbol=" + symbol + "&outputsize=full&apikey=" + alphaVantageApiKey;
            std::string data = httpGet(url);
            if (data.empty())
            {
                std::cerr << "Failed to fetch historical prices for " << symbol << "." << std::endl;
            }
            else
            {
                auto jsonData = nlohmann::json::parse(data, nullptr, false);
                if (jsonData.is_discarded())
                {
                    std::cerr << "Failed to parse historical prices JSON data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
                else if (jsonData.contains("Error Message") || jsonData.contains("Note"))
                {
                    std::cerr << "Alpha Vantage API Error (Historical Prices) for " << symbol << ": " << (jsonData.contains("Error Message") ? jsonData["Error Message"].get<std::string>() : jsonData["Note"].get<std::string>()) << std::endl;
                }
                else if (jsonData.contains("Time Series (Daily)"))
                {
                    auto timeSeries = jsonData["Time Series (Daily)"];
                    for (auto it = timeSeries.begin(); it != timeSeries.end(); ++it)
                    {
                        std::string dateStr = it.key(); // format: YYYY-MM-DD
                        if (dateStr >= startDate)
                        {
                            double adjustedClose = std::stod(it.value()["5. adjusted close"].get<std::string>());
                            historicalPrices.push_back(adjustedClose);
                        }
                    }
                }
                else
                {
                    std::cerr << "Unexpected JSON structure in historical prices data for " << symbol << "." << std::endl;
                    std::cerr << "Data: " << data << std::endl;
                }
            }
            // Sleep to respect API rate limits
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }

        double sharpeRatio = 0.0;
        double cagr = 0.0;
        double var = 0.0;
        double cvar = 0.0;

        if (historicalPrices.size() >= 252)
        {
            std::vector<double> returns;
            for (size_t i = 1; i < historicalPrices.size(); ++i)
            {
                double dailyReturn = (historicalPrices[i - 1] - historicalPrices[i]) / historicalPrices[i];
                returns.push_back(dailyReturn);
            }

            double avgReturn = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

            double stdDev = 0.0;
            for (double r : returns)
                stdDev += pow(r - avgReturn, 2);
            stdDev = sqrt(stdDev / (returns.size() - 1));

            double annualizedReturn = avgReturn * 252;
            double annualizedStdDev = stdDev * sqrt(252);

            // Use the latest available interest rate as risk-free rate
            double riskFreeRate = 0.0;
            if (!interestRates.empty())
                riskFreeRate = interestRates.rbegin()->second / 100.0;
            if (annualizedStdDev != 0)
                sharpeRatio = (annualizedReturn - riskFreeRate) / annualizedStdDev;

            double endingValue = historicalPrices.front();
            double beginningValue = historicalPrices.back();
            double yearsInvested = historicalPrices.size() / 252.0;
            cagr = pow((endingValue / beginningValue), (1.0 / yearsInvested)) - 1;

            std::sort(returns.begin(), returns.end());
            size_t index = static_cast<size_t>(0.05 * returns.size());
            var = returns[index];

            double sumLosses = std::accumulate(returns.begin(), returns.begin() + index + 1, 0.0);
            cvar = sumLosses / (index + 1);
        }

        double pricingDCF = 0.0; // Placeholder for DCF calculation

        // Write data to CSV for each month in lastMonths
        for (const auto& month : lastMonths)
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
    }

    csvFile.close();
    std::cout << "Data collection complete. Check 'financial_data.csv'." << std::endl;

    return 0;
}
