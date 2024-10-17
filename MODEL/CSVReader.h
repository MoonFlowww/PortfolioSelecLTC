#pragma once
#include <vector>
#include <string>
#include "FinancialData.h"

std::vector<FinancialData> loadFinancialData(const std::string& filename);
