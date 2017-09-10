#pragma once

#include <vector>
#include <functional>
#include <string>

std::vector<std::string> tokenizeText(const std::string& text, std::function<bool(const std::string&)> dictionaryContains);
