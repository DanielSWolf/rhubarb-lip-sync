#pragma once

#include <list>
#include <functional>
#include <mutex>
#include <vector>

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

class ProgressMerger {
public:
	ProgressMerger(ProgressSink& sink);
	ProgressSink& addSink(double weight);
private:
	void report();

	ProgressSink& sink;
	std::mutex mutex;
	double totalWeight = 0;
	std::list<ProgressForwarder> forwarders;
	std::vector<double> weightedValues;
};
