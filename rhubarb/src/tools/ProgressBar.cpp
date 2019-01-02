#include "ProgressBar.h"
#include <algorithm>
#include <future>
#include <chrono>
#include <format.h>
#include <boost/algorithm/clamp.hpp>
#include <cmath>

using std::string;

ProgressBar::ProgressBar(std::ostream& stream) :
	stream(stream)
{
	updateLoopFuture = std::async(std::launch::async, &ProgressBar::updateLoop, this);
}

ProgressBar::~ProgressBar() {
	done = true;
	updateLoopFuture.wait();
}

void ProgressBar::reportProgress(double value) {
	// Make sure value is in [0..1] range
	value = boost::algorithm::clamp(value, 0.0, 1.0);
	if (std::isnan(value)) {
		value = 0.0;
	}

	currentProgress = value;
}

void ProgressBar::updateLoop() {
	const int blockCount = 20;
	const std::chrono::milliseconds animationInterval(1000 / 8);
	const string animation = "|/-\\";

	while (!done) {
		int progressBlockCount = static_cast<int>(currentProgress * blockCount);
		int percent = static_cast<int>(currentProgress * 100);
		string text = fmt::format("[{0}{1}] {2:3}% {3}",
			string(progressBlockCount, '#'), string(blockCount - progressBlockCount, '-'),
			percent,
			animation[animationIndex++ % animation.size()]);
		updateText(text);

		std::this_thread::sleep_for(animationInterval);
	}

	if (clearOnDestruction) {
		updateText("");
	}
}

void ProgressBar::updateText(const string& text) {
	// Get length of common portion
	int commonPrefixLength = 0;
	int commonLength = std::min(currentText.size(), text.size());
	while (commonPrefixLength < commonLength && text[commonPrefixLength] == currentText[commonPrefixLength]) {
		commonPrefixLength++;
	}

	// Construct output string
	string output;

	// ... backtrack to the first differing character
	output.append(currentText.size() - commonPrefixLength, '\b');

	// ... add new suffix
	output.append(text, commonPrefixLength, text.size() - commonPrefixLength);

	// ... if the new text is shorter than the old one: delete overlapping characters
	int overlapCount = currentText.size() - text.size();
	if (overlapCount > 0) {
		output.append(overlapCount, ' ');
		output.append(overlapCount, '\b');
	}

	stream << output;
	currentText = text;
}
