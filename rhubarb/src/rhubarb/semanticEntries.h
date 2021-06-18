#pragma once
#include "logging/Entry.h"
#include <filesystem>

// Marker class for semantic entries
class SemanticEntry : public logging::Entry {
public:
	SemanticEntry(logging::Level level, const std::string& message);
};

class StartEntry : public SemanticEntry {
public:
	StartEntry(const std::filesystem::path& inputFilePath);
	std::filesystem::path getInputFilePath() const;
private:
	std::filesystem::path inputFilePath;
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
