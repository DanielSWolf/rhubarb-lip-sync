#pragma once

#include <functional>
#include "ProgressBar.h"
#include <gsl_util.h>

template<typename TCollection>
void runParallel(
	std::function<void(typename TCollection::reference)> processElement,
	TCollection& collection,
	int maxThreadCount)
{
	if (maxThreadCount < 1) {
		throw std::invalid_argument(fmt::format("maxThreadCount cannot be {}.", maxThreadCount));
	}

	if (maxThreadCount == 1) {
		// Process synchronously
		for (auto& element : collection) {
			processElement(element);
		}
		return;
	}

	using future_type = std::future<void>;

	std::mutex mutex;
	int currentThreadCount = 0;
	std::condition_variable elementFinished;
	future_type finishedElement;

	// Before exiting, wait for all running tasks to finish, but don't re-throw exceptions.
	// This only applies if one task already failed with an exception.
	auto finishRunning = gsl::finally([&]{
		std::unique_lock<std::mutex> lock(mutex);
		elementFinished.wait(lock, [&] { return currentThreadCount == 0; });
	});

	// Asyncronously run all elements
	for (auto it = collection.begin(); it != collection.end(); ++it) {
		// This variable will later hold the future, but can be value-captured right now
		auto future = std::make_shared<future_type>();

		// Notifies that an element is done processing
		auto notifyElementDone = [&, future] {
			std::lock_guard<std::mutex> lock(mutex);
			finishedElement = std::move(*future);
			--currentThreadCount;
			elementFinished.notify_one();
		};

		// Processes the current element, then notifies
		auto wrapperFunction = [processElement, &element = *it, notifyElementDone]() {
			auto done = gsl::finally(notifyElementDone);
			processElement(element);
		};

		// Asynchronously process element
		{
			std::lock_guard<std::mutex> lock(mutex);
			*future = std::async(std::launch::async, wrapperFunction);
			++currentThreadCount;
		}

		// Wait for threads to finish, if necessary
		{
			std::unique_lock<std::mutex> lock(mutex);
			int targetThreadCount = it == collection.end() ? 0 : maxThreadCount - 1;
			while (currentThreadCount > targetThreadCount) {
				elementFinished.wait(lock);
				if (finishedElement.valid()) {
					// Re-throw any exception
					finishedElement.get();
					finishedElement = future_type();
				}
			}
		}
	}

}

template<typename TCollection>
void runParallel(
	std::function<void(typename TCollection::reference, ProgressSink&)> processElement,
	TCollection& collection,
	int maxThreadCount,
	ProgressSink& progressSink,
	std::function<double(const typename TCollection::reference)> getElementProgressWeight = [](typename TCollection::reference) { return 1.0; })
{
	// Create a collection of wrapper functions that take care of progress handling
	ProgressMerger progressMerger(progressSink);
	std::vector<std::function<void()>> functions;
	for (auto& element : collection) {
		auto& elementProgressSink = progressMerger.addSink(getElementProgressWeight(element));
		functions.push_back([&]() { processElement(element, elementProgressSink); });
	}

	// Run wrapper function
	runParallel([&](std::function<void()> function) { function(); }, functions, maxThreadCount);
}

inline int getProcessorCoreCount() {
	int coreCount = std::thread::hardware_concurrency();

	// If the number of cores cannot be determined, use a reasonable default
	return coreCount != 0 ? coreCount : 4;
}
