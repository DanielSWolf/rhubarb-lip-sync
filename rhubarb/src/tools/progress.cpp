#include "progress.h"

#include <mutex>

using std::string;

ProgressForwarder::ProgressForwarder(std::function<void(double progress)> callback) :
	callback(callback)
{}

void ProgressForwarder::reportProgress(double value) {
	callback(value);
}

ProgressMerger::ProgressMerger(ProgressSink& sink) :
	sink(sink)
{}

ProgressSink& ProgressMerger::addSink(double weight) {
	std::lock_guard<std::mutex> lock(mutex);

	totalWeight += weight;
	int sinkIndex = weightedValues.size();
	weightedValues.push_back(0);
	forwarders.emplace_back([weight, sinkIndex, this](double progress) {
		weightedValues[sinkIndex] = progress * weight;
		report();
	});
	return forwarders.back();
}

void ProgressMerger::report() {
	std::lock_guard<std::mutex> lock(mutex);

	if (totalWeight != 0) {
		double weightedSum = 0;
		for (double weightedValue : weightedValues) {
			weightedSum += weightedValue;
		}
		const double progress = weightedSum / totalWeight;
		sink.reportProgress(progress);
	} else {
		sink.reportProgress(0);
	}
}
