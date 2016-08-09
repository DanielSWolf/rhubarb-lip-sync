#include "tools.h"
#include "platformTools.h"
#include <format.h>
#include <chrono>
#include <vector>

using std::string;
using std::chrono::duration;

string formatDuration(duration<double> seconds) {
	return fmt::format("{0:.2f}", seconds.count());
}

string formatTime(time_t time, const string& format) {
	tm timeInfo = getLocalTime(time);
	std::vector<char> buffer(20);
	bool success = false;
	while (!success) {
		success = strftime(buffer.data(), buffer.size(), format.c_str(), &timeInfo) != 0;
		if (!success) buffer.resize(buffer.size() * 2);
	}
	return string(buffer.data());
}
