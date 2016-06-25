#pragma once

#include <vector>
#include <functional>

std::vector<std::string> tokenizeText(const std::u32string& text, std::function<bool(const std::string&)> dictionaryContains);
