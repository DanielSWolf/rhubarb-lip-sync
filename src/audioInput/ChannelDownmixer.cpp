#include "ChannelDownmixer.h"

ChannelDownmixer::ChannelDownmixer(std::unique_ptr<AudioStream> inputStream) :
	inputStream(std::move(inputStream)),
	inputChannelCount(this->inputStream->getChannelCount())
{}

int ChannelDownmixer::getFrameRate() {
	return inputStream->getFrameRate();
}

int ChannelDownmixer::getFrameCount() {
	return inputStream->getFrameCount();
}

int ChannelDownmixer::getChannelCount() {
	return 1;
}

bool ChannelDownmixer::getNextSample(float &sample) {
	float sum = 0;
	for (int channelIndex = 0; channelIndex < inputChannelCount; channelIndex++) {
		float currentSample;
		if (!inputStream->getNextSample(currentSample)) return false;

		sum += currentSample;
	}

	sample = sum / inputChannelCount;
	return true;
}
