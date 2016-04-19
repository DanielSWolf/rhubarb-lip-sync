#include "AudioStreamSegment.h"
#include <stdexcept>

AudioStreamSegment::AudioStreamSegment(std::unique_ptr<AudioStream> audioStream, const TimeRange& range) :
	audioStream(std::move(audioStream))
{
	sampleOffset = range.getStart().count() * this->audioStream->getSampleRate() / 100;
	sampleCount = range.getLength().count() * this->audioStream->getSampleRate() / 100;
	seek(0);

	if (sampleOffset < 0 || sampleOffset + sampleCount > this->audioStream->getSampleCount()) {
		throw std::invalid_argument("Segment extends beyond input stream.");
	}
}

AudioStreamSegment::AudioStreamSegment(const AudioStreamSegment& rhs, bool reset) :
	audioStream(rhs.audioStream->clone(false)),
	sampleOffset(rhs.sampleOffset),
	sampleCount(rhs.sampleCount)
{
	if (reset) seek(0);
}

std::unique_ptr<AudioStream> AudioStreamSegment::clone(bool reset) const {
	return std::make_unique<AudioStreamSegment>(*this, reset);
}

int AudioStreamSegment::getSampleRate() const {
	return audioStream->getSampleRate();
}

int AudioStreamSegment::getSampleCount() const {
	return sampleCount;
}

int AudioStreamSegment::getSampleIndex() const {
	return audioStream->getSampleIndex() - sampleOffset;
}

void AudioStreamSegment::seek(int sampleIndex) {
	audioStream->seek(sampleIndex + sampleOffset);
}

float AudioStreamSegment::readSample() {
	return audioStream->readSample();
}

std::unique_ptr<AudioStream> createSegment(std::unique_ptr<AudioStream> audioStream, const TimeRange& range) {
	return std::make_unique<AudioStreamSegment>(std::move(audioStream), range);
}
