#pragma once

#include <functional>
#include <mutex>
#include <vector>
#include <memory>
#include <string>

class ProgressSink {
public:
	virtual ~ProgressSink() {}
	virtual void reportProgress(double value) = 0;
};

class NullProgressSink : public ProgressSink {
public:
	void reportProgress(double) override {}
};

class ProgressForwarder : public ProgressSink {
public:
	ProgressForwarder(std::function<void(double progress)> callback);
	void reportProgress(double value) override;
private:
	std::function<void(double progress)> callback;
};

struct MergerSource {
	std::string description;
	double weight;
	// Needs to be a pointer because we give away references to the forwarder
	// itself which would become invalid if the MergerSource is moved.
	std::unique_ptr<ProgressForwarder> forwarder;
	double progress;
};

class ProgressMerger {
public:
	ProgressMerger(ProgressSink& sink);
	~ProgressMerger();
	ProgressSink& addSource(const std::string& description, double weight);
private:
	void report();

	ProgressSink& sink;
	std::mutex mutex;
	double totalWeight = 0;
	std::vector<MergerSource> sources;
};
