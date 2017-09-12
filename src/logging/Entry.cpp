#include "Entry.h"

#include <thread>
#include <mutex>
#include <unordered_map>

using std::lock_guard;
using std::unordered_map;
using std::string;

namespace logging {

	// Returns an int representing the current thread.
	// This used to be a simple thread_local variable, but Xcode doesn't support that yet
	int getThreadCounter() {
		using thread_id = std::thread::id;

		static std::mutex counterMutex;
		lock_guard<std::mutex> lock(counterMutex);

		static unordered_map<thread_id, int> threadCounters;
		static int lastThreadId = 0;
		thread_id threadId = std::this_thread::get_id();
		if (threadCounters.find(threadId) == threadCounters.end()) {
			threadCounters.insert({threadId, ++lastThreadId});
		}
		return threadCounters.find(threadId)->second;
	}

	Entry::Entry(Level level, const string& message) :
		level(level),
		message(message)
	{
		time(&timestamp);
		this->threadCounter = getThreadCounter();
	}

}
