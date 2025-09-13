#pragma once

#include <functional>
#include <string>
#include <vector>

std::vector<std::string> tokenizeText(
    const std::string& text, const std::function<bool(const std::string&)>& dictionaryContains
);
