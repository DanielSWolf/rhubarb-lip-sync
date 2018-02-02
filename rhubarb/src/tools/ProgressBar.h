#pragma once

#include <atomic>
#include <future>
#include <functional>
#include <list>
#include <vector>
#include <mutex>
#include <string>
#include <iostream>

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

class ProgressBar : public ProgressSink {
public:
	ProgressBar(std::ostream& stream = std::cerr);
	~ProgressBar();
	void reportProgress(double value) override;

	bool getClearOnDestruction() const {
		return clearOnDestruction;
	}

	void setClearOnDestruction(bool value) {
		clearOnDestruction = value;
	}

private:
	void updateLoop();
	void updateText(const std::string& text);

	std::future<void> updateLoopFuture;
	std::atomic<double> currentProgress { 0 };
	std::atomic<bool> done { false };

	std::ostream& stream;
	std::string currentText;
	int animationIndex = 0;
	bool clearOnDestruction = true;
};
