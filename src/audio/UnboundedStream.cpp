#include "UnboundedStream.h"

using boost::optional;

UnboundedStream::UnboundedStream(std::unique_ptr<AudioStream> inputStream) :
	innerStream(std::move(innerStream)),
	sampleIndex(innerStream->getSampleIndex()),
	firstSample(inputStream->getSampleCount() ? optional<float>() : 0.0f),
	lastSample(inputStream->getSampleCount() ? optional<float>() : 0.0f)
{}

UnboundedStream::UnboundedStream(const UnboundedStream& rhs, bool reset) :
	innerStream(rhs.innerStream->clone(reset)),
	sampleIndex(rhs.sampleIndex),
	firstSample(rhs.firstSample),
	lastSample(rhs.lastSample)
{}

std::unique_ptr<AudioStream> UnboundedStream::clone(bool reset) {
	return std::make_unique<UnboundedStream>(*this, reset);
}

int UnboundedStream::getSampleRate() {
	return innerStream->getSampleRate();
}

int UnboundedStream::getSampleCount() {
	return innerStream->getSampleCount();
}

int UnboundedStream::getSampleIndex() {
	return sampleIndex;
}

void UnboundedStream::seek(int sampleIndex) {
	this->sampleIndex = sampleIndex;
}

float UnboundedStream::readSample() {
	if (sampleIndex < 0) {
		if (!firstSample) {
			innerStream->seek(0);
			firstSample = innerStream->readSample();
		}
		return firstSample.get();
	}
	if (sampleIndex >= innerStream->getSampleCount()) {
		if (!lastSample) {
			innerStream->seek(innerStream->getSampleCount() - 1);
			lastSample = innerStream->readSample();
		}
		return lastSample.get();
	}

	if (sampleIndex != innerStream->getSampleIndex()) {
		innerStream->seek(sampleIndex);
	}
	return innerStream->readSample();
}
