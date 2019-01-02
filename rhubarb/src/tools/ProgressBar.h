#pragma once

#include <atomic>
#include <future>
#include <iostream>
#include "progress.h"

class ProgressBar : public ProgressSink {
public:
	ProgressBar(std::ostream& stream = std::cerr);
	~ProgressBar();
	void reportProgress(double value) override;

	bool getClearOnDestruction() const {
		return clearOnDestruction;
	}

	void setClearOnDestruction(bool value) {
		clearOnDestruction = value;
	}

private:
	void updateLoop();
	void updateText(const std::string& text);

	std::future<void> updateLoopFuture;
	std::atomic<double> currentProgress { 0 };
	std::atomic<bool> done { false };

	std::ostream& stream;
	std::string currentText;
	int animationIndex = 0;
	bool clearOnDestruction = true;
};
