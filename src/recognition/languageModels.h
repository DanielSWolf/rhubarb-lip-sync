#pragma once

#include <vector>
#include "tools/tools.h"

extern "C" {
#include <pocketsphinx.h>
#include <ngram_search.h>
}

lambda_unique_ptr<ngram_model_t> createLanguageModel(const std::vector<std::string>& words, ps_decoder_t& decoder);
