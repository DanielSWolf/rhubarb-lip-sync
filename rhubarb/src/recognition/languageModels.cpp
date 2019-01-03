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
using std::get;
using std::endl;
using boost::filesystem::path;

using Unigram = string;
using Bigram = tuple<string, string>;
using Trigram = tuple<string, string, string>;

map<Unigram, int> getUnigramCounts(const vector<string>& words) {
	map<Unigram, int> unigramCounts;
	for (const Unigram& unigram : words) {
		++unigramCounts[unigram];
	}
	return unigramCounts;
}

map<Bigram, int> getBigramCounts(const vector<string>& words) {
	map<Bigram, int> bigramCounts;
	for (auto it = words.begin(); it < words.end() - 1; ++it) {
		++bigramCounts[Bigram(*it, *(it + 1))];
	}
	return bigramCounts;
}

map<Trigram, int> getTrigramCounts(const vector<string>& words) {
	map<Trigram, int> trigramCounts;
	if (words.size() >= 3) {
		for (auto it = words.begin(); it < words.end() - 2; ++it) {
			++trigramCounts[Trigram(*it, *(it + 1), *(it + 2))];
		}
	}
	return trigramCounts;
}

map<Unigram, double> getUnigramProbabilities(
	const vector<string>& words,
	const map<Unigram, int>& unigramCounts,
	const double deflator
) {
	map<Unigram, double> unigramProbabilities;
	for (const auto& pair : unigramCounts) {
		const Unigram& unigram = get<0>(pair);
		const int unigramCount = get<1>(pair);
		unigramProbabilities[unigram] = double(unigramCount) / words.size() * deflator;
	}
	return unigramProbabilities;
}

map<Bigram, double> getBigramProbabilities(
	const map<Unigram, int>& unigramCounts,
	const map<Bigram, int>& bigramCounts,
	const double deflator
) {
	map<Bigram, double> bigramProbabilities;
	for (const auto& pair : bigramCounts) {
		Bigram bigram = get<0>(pair);
		const int bigramCount = get<1>(pair);
		const int unigramPrefixCount = unigramCounts.at(get<0>(bigram));
		bigramProbabilities[bigram] = double(bigramCount) / unigramPrefixCount * deflator;
	}
	return bigramProbabilities;
}

map<Trigram, double> getTrigramProbabilities(
	const map<Bigram, int>& bigramCounts,
	const map<Trigram, int>& trigramCounts,
	const double deflator
) {
	map<Trigram, double> trigramProbabilities;
	for (const auto& pair : trigramCounts) {
		Trigram trigram = get<0>(pair);
		const int trigramCount = get<1>(pair);
		const int bigramPrefixCount = bigramCounts.at(Bigram(get<0>(trigram), get<1>(trigram)));
		trigramProbabilities[trigram] = double(trigramCount) / bigramPrefixCount * deflator;
	}
	return trigramProbabilities;
}

map<Unigram, double> getUnigramBackoffWeights(
	const map<Unigram, int>& unigramCounts,
	const map<Unigram, double>& unigramProbabilities,
	const map<Bigram, int>& bigramCounts,
	const double discountMass)
{
	map<Unigram, double> unigramBackoffWeights;
	for (const Unigram& unigram : unigramCounts | boost::adaptors::map_keys) {
		double denominator = 1;
		for (const Bigram& bigram : bigramCounts | boost::adaptors::map_keys) {
			if (get<0>(bigram) == unigram) {
				denominator -= unigramProbabilities.at(get<1>(bigram));
			}
		}
		unigramBackoffWeights[unigram] = discountMass / denominator;
	}
	return unigramBackoffWeights;
}

map<Bigram, double> getBigramBackoffWeights(
	const map<Bigram, int>& bigramCounts,
	const map<Bigram, double>& bigramProbabilities,
	const map<Trigram, int>& trigramCounts,
	const double discountMass)
{
	map<Bigram, double> bigramBackoffWeights;
	for (const Bigram& bigram : bigramCounts | boost::adaptors::map_keys) {
		double denominator = 1;
		for (const Trigram& trigram : trigramCounts | boost::adaptors::map_keys) {
			if (Bigram(get<0>(trigram), get<1>(trigram)) == bigram) {
				denominator -= bigramProbabilities.at(Bigram(get<1>(trigram), get<2>(trigram)));
			}
		}
		bigramBackoffWeights[bigram] = discountMass / denominator;
	}
	return bigramBackoffWeights;
}

void createLanguageModelFile(const vector<string>& words, const path& filePath) {
	const double discountMass = 0.5;
	const double deflator = 1.0 - discountMass;

	map<Unigram, int> unigramCounts = getUnigramCounts(words);
	map<Bigram, int> bigramCounts = getBigramCounts(words);
	map<Trigram, int> trigramCounts = getTrigramCounts(words);

	map<Unigram, double> unigramProbabilities =
		getUnigramProbabilities(words, unigramCounts, deflator);
	map<Bigram, double> bigramProbabilities =
		getBigramProbabilities(unigramCounts, bigramCounts, deflator);
	map<Trigram, double> trigramProbabilities =
		getTrigramProbabilities(bigramCounts, trigramCounts, deflator);

	map<Unigram, double> unigramBackoffWeights =
		getUnigramBackoffWeights(unigramCounts, unigramProbabilities, bigramCounts, discountMass);
	map<Bigram, double> bigramBackoffWeights =
		getBigramBackoffWeights(bigramCounts, bigramProbabilities, trigramCounts, discountMass);

	boost::filesystem::ofstream file(filePath);
	file << "Generated by " << appName << " " << appVersion << endl << endl;

	file << "\\data\\" << endl;
	file << "ngram 1=" << unigramCounts.size() << endl;
	file << "ngram 2=" << bigramCounts.size() << endl;
	file << "ngram 3=" << trigramCounts.size() << endl << endl;

	file.setf(std::ios::fixed, std::ios::floatfield);
	file.precision(4);
	file << "\\1-grams:" << endl;
	for (const Unigram& unigram : unigramCounts | boost::adaptors::map_keys) {
		file << log10(unigramProbabilities.at(unigram))
			<< " " << unigram
			<< " " << log10(unigramBackoffWeights.at(unigram)) << endl;
	}
	file << endl;

	file << "\\2-grams:" << endl;
	for (const Bigram& bigram : bigramCounts | boost::adaptors::map_keys) {
		file << log10(bigramProbabilities.at(bigram))
			<< " " << get<0>(bigram) << " " << get<1>(bigram)
			<< " " << log10(bigramBackoffWeights.at(bigram)) << endl;
	}
	file << endl;

	file << "\\3-grams:" << endl;
	for (const Trigram& trigram : trigramCounts | boost::adaptors::map_keys) {
		file << log10(trigramProbabilities.at(trigram))
			<< " " << get<0>(trigram) << " " << get<1>(trigram) << " " << get<2>(trigram) << endl;
	}
	file << endl;

	file << "\\end\\" << endl;
}

lambda_unique_ptr<ngram_model_t> createLanguageModel(
	const vector<string>& words,
	ps_decoder_t& decoder
) {
	path tempFilePath = getTempFilePath();
	createLanguageModelFile(words, tempFilePath);
	auto deleteTempFile = gsl::finally([&]() { boost::filesystem::remove(tempFilePath); });

	return lambda_unique_ptr<ngram_model_t>(
		ngram_model_read(decoder.config, tempFilePath.string().c_str(), NGRAM_ARPA, decoder.lmath),
		[](ngram_model_t* lm) { ngram_model_free(lm); });
}
