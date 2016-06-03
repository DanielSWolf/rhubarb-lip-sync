#pragma once
#include <sphinxbase/ngram_model.h>
#include <vector>
#include "tools.h"

lambda_unique_ptr<ngram_model_t> createLanguageModel(const std::vector<std::string>& words, logmath_t& logMath);
