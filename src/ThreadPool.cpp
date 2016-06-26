#include "ThreadPool.h"

int ThreadPool::getRecommendedThreadCount() {
	int coreCount = std::thread::hardware_concurrency();

	// If the number of cores cannot be determined, use a reasonable default
	return coreCount != 0 ? coreCount : 4;
}

ThreadPool::ThreadPool(int threadCount) :
	threadCount(threadCount),
	remainingJobCount(0),
	bailout(false) {
	for (int i = 0; i < threadCount; ++i) {
		threads.push_back(std::thread([&] {
			Task();
		}));
	}
}

ThreadPool::~ThreadPool() {
	waitAll();

	// Notify that we're done, and wake up any threads that are waiting for a new job
	bailout = true;
	jobAvailableCondition.notify_all();

	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void ThreadPool::addJob(job_t job) {
	std::lock_guard<std::mutex> guard(queueMutex);
	jobQueue.emplace_back(job);
	++remainingJobCount;
	jobAvailableCondition.notify_one();
}

void ThreadPool::waitAll() {
	if (remainingJobCount == 0) return;

	std::unique_lock<std::mutex> lock(waitMutex);
	waitCondition.wait(lock, [&] {
		return remainingJobCount == 0;
	});
	lock.unlock();
}

void ThreadPool::Task() {
	while (!bailout) {
		getNextJob()();
		--remainingJobCount;
		waitCondition.notify_one();
	}
}

ThreadPool::job_t ThreadPool::getNextJob() {
	std::unique_lock<std::mutex> jobLock(queueMutex);

	// Wait for a job if we don't have any
	jobAvailableCondition.wait(jobLock, [&] {
		return jobQueue.size() > 0 || bailout;
	});

	if (bailout) {
		// Return a dummy job to keep remainingJobCount accurate
		++remainingJobCount;
		return [] {};
	}

	// Get job from the queue
	auto result = jobQueue.front();
	jobQueue.pop_front();
	return result;
}
