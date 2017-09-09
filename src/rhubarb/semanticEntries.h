#pragma once
#include "logging/Entry.h"
#include <boost/filesystem/path.hpp>

// Marker class for semantic entries
class SemanticEntry : public logging::Entry {
public:
	SemanticEntry(logging::Level level, const std::string& message);
};

class StartEntry : public SemanticEntry {
public:
	StartEntry(const boost::filesystem::path& inputFilePath);
	boost::filesystem::path getInputFilePath() const;
private:
	boost::filesystem::path inputFilePath;
};

class ProgressEntry : public SemanticEntry {
public:
	ProgressEntry(double progress);
	double getProgress() const;
private:
	double progress;
};

class SuccessEntry : public SemanticEntry {
public:
	SuccessEntry();
};

class FailureEntry : public SemanticEntry {
public:
	FailureEntry(const std::string& reason);
	std::string getReason() const;
private:
	std::string reason;
};
