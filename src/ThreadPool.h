#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <vector>

// Thread pool based on https://github.com/nbsdx/ThreadPool, which is in the public domain.

class ThreadPool {
public:
	using job_t = std::function<void(void)>;

	static int getRecommendedThreadCount();

	ThreadPool(int threadCount = getRecommendedThreadCount());

	~ThreadPool();

	// Gets the number of threads in this pool
	int getThreadCount() const {
		return threadCount;
	}

	// Gets the number of jobs left in the queue
	int getRemainingJobCount() const {
		return remainingJobCount;
	}

	// Adds a new job to the pool.
	// If there are no queued jobs, a thread is woken up to take the job.
	// If all threads are busy, the job is added to the end of the queue.
	void addJob(job_t job);

	// Blocks until all jobs have finshed executing
	void waitAll();

private:
	const int threadCount;
	std::vector<std::thread> threads;
	std::list<job_t> jobQueue;
	std::atomic_int remainingJobCount; // The number of queued or running jobs
	std::atomic_bool bailout;
	std::condition_variable jobAvailableCondition;
	std::condition_variable waitCondition;
	std::mutex waitMutex;
	std::mutex queueMutex;

	// Takes the next job in the queue and run it.
	// Notify the main thread that a job has completed.
	void Task();

	// Gets the next job; pop the first item in the queue,
	// otherwise wait for a signal from the main thread
	job_t getNextJob();
};

