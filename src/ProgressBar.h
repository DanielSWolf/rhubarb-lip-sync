#pragma once

#include <string>
#include <atomic>
#include <future>
#include <functional>
#include <list>
#include <vector>
#include <mutex>

class ProgressSink {
public:
	virtual ~ProgressSink() {}
	virtual void reportProgress(double value) = 0;
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

class ProgressBar : public ProgressSink {
public:
	ProgressBar();
	~ProgressBar();
	void reportProgress(double value) override;

private:
	void updateLoop();
	void updateText(const std::string& text);

	std::future<void> updateLoopFuture;
	std::atomic<double> currentProgress { 0 };
	std::atomic<bool> done { false };

	std::string currentText;
	int animationIndex = 0;
};
