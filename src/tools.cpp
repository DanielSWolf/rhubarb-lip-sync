#include "tools.h"
#include <format.h>
#include <chrono>

using std::string;
using std::chrono::duration;

string formatDuration(duration<double> seconds) {
	return fmt::format("{0:.2f}", seconds.count());
}
