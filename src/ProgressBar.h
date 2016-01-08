#pragma once

#include <string>
#include <atomic>
#include <future>

class ProgressBar {
public:
	ProgressBar();
	~ProgressBar();
	void reportProgress(double value);

private:
	void updateLoop();
	void updateText(const std::string& text);

	std::future<void> updateLoopFuture;
	std::atomic<double> currentProgress { 0 };
	std::atomic<bool> done { false };

	std::string currentText;
	int animationIndex = 0;
};


