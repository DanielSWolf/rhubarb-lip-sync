#include "languageModels.h"
#include <boost/range/adaptor/map.hpp>
#include <vector>
#include <regex>
#include <map>
#include <tuple>
#include "tools/platformTools.h"
#include <boost/filesystem/fstream.hpp>
#include "core/appInfo.h"
#include <cmath>
#include <gsl_util.h>

using std::string;
using std::vector;
using std::regex;
using std::map;
using std::tuple;
using std::make_tuple;
using std::get;
using std::endl;
using boost::filesystem::path;

using unigram_t = string;
using bigram_t = tuple<string, string>;
using trigram_t = tuple<string, string, string>;

map<unigram_t, int> getUnigramCounts(const vector<string>& words) {
	map<unigram_t, int> unigramCounts;
	for (const unigram_t& unigram : words) {
		++unigramCounts[unigram];
	}
	return unigramCounts;
}

map<bigram_t, int> getBigramCounts(const vector<string>& words) {
	map<bigram_t, int> bigramCounts;
	for (auto it = words.begin(); it < words.end() - 1; ++it) {
		++bigramCounts[bigram_t(*it, *(it + 1))];
	}
	return bigramCounts;
}

map<trigram_t, int> getTrigramCounts(const vector<string>& words) {
	map<trigram_t, int> trigramCounts;
	if (words.size() >= 3) {
		for (auto it = words.begin(); it < words.end() - 2; ++it) {
			++trigramCounts[trigram_t(*it, *(it + 1), *(it + 2))];
		}
	}
	return trigramCounts;
}

map<unigram_t, double> getUnigramProbabilities(const vector<string>& words, const map<unigram_t, int>& unigramCounts, const double deflator) {
	map<unigram_t, double> unigramProbabilities;
	for (const auto& pair : unigramCounts) {
		unigram_t unigram = get<0>(pair);
		int unigramCount = get<1>(pair);
		unigramProbabilities[unigram] = double(unigramCount) / words.size() * deflator;
	}
	return unigramProbabilities;
}

map<bigram_t, double> getBigramProbabilities(const map<unigram_t, int>& unigramCounts, const map<bigram_t, int>& bigramCounts, const double deflator) {
	map<bigram_t, double> bigramProbabilities;
	for (const auto& pair : bigramCounts) {
		bigram_t bigram = get<0>(pair);
		int bigramCount = get<1>(pair);
		int unigramPrefixCount = unigramCounts.at(get<0>(bigram));
		bigramProbabilities[bigram] = double(bigramCount) / unigramPrefixCount * deflator;
	}
	return bigramProbabilities;
}

map<trigram_t, double> getTrigramProbabilities(const map<bigram_t, int>& bigramCounts, const map<trigram_t, int>& trigramCounts, const double deflator) {
	map<trigram_t, double> trigramProbabilities;
	for (const auto& pair : trigramCounts) {
		trigram_t trigram = get<0>(pair);
		int trigramCount = get<1>(pair);
		int bigramPrefixCount = bigramCounts.at(bigram_t(get<0>(trigram), get<1>(trigram)));
		trigramProbabilities[trigram] = double(trigramCount) / bigramPrefixCount * deflator;
	}
	return trigramProbabilities;
}

map<unigram_t, double> getUnigramBackoffWeights(
	const map<unigram_t, int>& unigramCounts,
	const map<unigram_t, double>& unigramProbabilities,
	const map<bigram_t, int>& bigramCounts,
	const double discountMass)
{
	map<unigram_t, double> unigramBackoffWeights;
	for (const unigram_t& unigram : unigramCounts | boost::adaptors::map_keys) {
		double denominator = 1;
		for (const bigram_t& bigram : bigramCounts | boost::adaptors::map_keys) {
			if (get<0>(bigram) == unigram) {
				denominator -= unigramProbabilities.at(get<1>(bigram));
			}
		}
		unigramBackoffWeights[unigram] = discountMass / denominator;
	}
	return unigramBackoffWeights;
}

map<bigram_t, double> getBigramBackoffWeights(
	const map<bigram_t, int>& bigramCounts,
	const map<bigram_t, double>& bigramProbabilities,
	const map<trigram_t, int>& trigramCounts,
	const double discountMass)
{
	map<bigram_t, double> bigramBackoffWeights;
	for (const bigram_t& bigram : bigramCounts | boost::adaptors::map_keys) {
		double denominator = 1;
		for (const trigram_t& trigram : trigramCounts | boost::adaptors::map_keys) {
			if (bigram_t(get<0>(trigram), get<1>(trigram)) == bigram) {
				denominator -= bigramProbabilities.at(bigram_t(get<1>(trigram), get<2>(trigram)));
			}
		}
		bigramBackoffWeights[bigram] = discountMass / denominator;
	}
	return bigramBackoffWeights;
}

void createLanguageModelFile(const vector<string>& words, path filePath) {
	const double discountMass = 0.5;
	const double deflator = 1.0 - discountMass;

	map<unigram_t, int> unigramCounts = getUnigramCounts(words);
	map<bigram_t, int> bigramCounts = getBigramCounts(words);
	map<trigram_t, int> trigramCounts = getTrigramCounts(words);

	map<unigram_t, double> unigramProbabilities = getUnigramProbabilities(words, unigramCounts, deflator);
	map<bigram_t, double> bigramProbabilities = getBigramProbabilities(unigramCounts, bigramCounts, deflator);
	map<trigram_t, double> trigramProbabilities = getTrigramProbabilities(bigramCounts, trigramCounts, deflator);

	map<unigram_t, double> unigramBackoffWeights = getUnigramBackoffWeights(unigramCounts, unigramProbabilities, bigramCounts, discountMass);
	map<bigram_t, double> bigramBackoffWeights = getBigramBackoffWeights(bigramCounts, bigramProbabilities, trigramCounts, discountMass);

	boost::filesystem::ofstream file(filePath);
	file << "Generated by " << appName << " " << appVersion << endl << endl;

	file << "\\data\\" << endl;
	file << "ngram 1=" << unigramCounts.size() << endl;
	file << "ngram 2=" << bigramCounts.size() << endl;
	file << "ngram 3=" << trigramCounts.size() << endl << endl;

	file.setf(std::ios::fixed, std::ios::floatfield);
	file.precision(4);
	file << "\\1-grams:" << endl;
	for (const unigram_t& unigram : unigramCounts | boost::adaptors::map_keys) {
		file << log10(unigramProbabilities.at(unigram))
			<< " " << unigram
			<< " " << log10(unigramBackoffWeights.at(unigram)) << endl;
	}
	file << endl;

	file << "\\2-grams:" << endl;
	for (const bigram_t& bigram : bigramCounts | boost::adaptors::map_keys) {
		file << log10(bigramProbabilities.at(bigram))
			<< " " << get<0>(bigram) << " " << get<1>(bigram)
			<< " " << log10(bigramBackoffWeights.at(bigram)) << endl;
	}
	file << endl;

	file << "\\3-grams:" << endl;
	for (const trigram_t& trigram : trigramCounts | boost::adaptors::map_keys) {
		file << log10(trigramProbabilities.at(trigram))
			<< " " << get<0>(trigram) << " " << get<1>(trigram) << " " << get<2>(trigram) << endl;
	}
	file << endl;

	file << "\\end\\" << endl;
}

lambda_unique_ptr<ngram_model_t> createLanguageModel(const vector<string>& words, ps_decoder_t& decoder) {
	path tempFilePath = getTempFilePath();
	createLanguageModelFile(words, tempFilePath);
	auto deleteTempFile = gsl::finally([&]() { boost::filesystem::remove(tempFilePath); });

	return lambda_unique_ptr<ngram_model_t>(
		ngram_model_read(decoder.config, tempFilePath.string().c_str(), NGRAM_ARPA, decoder.lmath),
		[](ngram_model_t* lm) { ngram_model_free(lm); });
}
