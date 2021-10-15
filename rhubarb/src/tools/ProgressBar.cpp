#include "ProgressBar.h"

#include <algorithm>
#include <future>
#include <chrono>
#include <format.h>
#include <boost/algorithm/clamp.hpp>
#include <cmath>
#include <thread>

using std::string;

double sanitizeProgress(double progress) {
	// Make sure value is in [0..1] range
	return std::isnan(progress)
		? 0.0
		: boost::algorithm::clamp(progress, 0.0, 1.0);
}

ProgressBar::ProgressBar(double progress) :
	ProgressBar(std::cerr, progress)
{}

ProgressBar::ProgressBar(std::ostream& stream, double progress) :
	stream(stream)
{
	currentProgress = sanitizeProgress(progress);
	updateLoopFuture = std::async(std::launch::async, &ProgressBar::updateLoop, this);
}

ProgressBar::~ProgressBar() {
	done = true;
	updateLoopFuture.wait();
}

void ProgressBar::reportProgress(double value) {
	currentProgress = sanitizeProgress(value);
}

void ProgressBar::updateLoop() {
	const std::chrono::milliseconds animationInterval(1000 / 8);

	while (!done) {
		update();
		std::this_thread::sleep_for(animationInterval);
	}

	if (clearOnDestruction) {
		updateText("");
	} else {
		update(false);
	}
}

void ProgressBar::update(bool showSpinner) {
	const int blockCount = 20;
	const string animation = "|/-\\";

	const int progressBlockCount = static_cast<int>(currentProgress * blockCount);
	const double epsilon = 0.0001;
	const int percent = static_cast<int>(currentProgress * 100 + epsilon);
	const string spinner = showSpinner
		? string(1, animation[animationIndex++ % animation.size()])
		: "";
	const string text = fmt::format("[{0}{1}] {2:3}% {3}",
		string(progressBlockCount, '#'), string(blockCount - progressBlockCount, '-'),
		percent,
		spinner
	);
	updateText(text);
}

void ProgressBar::updateText(const string& text) {
	// Get length of common portion
	size_t commonPrefixLength = 0;
	const size_t commonLength = std::min(currentText.size(), text.size());
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
	const int overlapCount = static_cast<int>(currentText.size()) - static_cast<int>(text.size());
	if (overlapCount > 0) {
		output.append(overlapCount, ' ');
		output.append(overlapCount, '\b');
	}

	stream << output;
	currentText = text;
}
