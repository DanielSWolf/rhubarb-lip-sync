#include "UnboundedStream.h"

using boost::optional;

UnboundedStream::UnboundedStream(std::unique_ptr<AudioStream> inputStream) :
	innerStream(std::move(inputStream)),
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

std::unique_ptr<AudioStream> UnboundedStream::clone(bool reset) const {
	return std::make_unique<UnboundedStream>(*this, reset);
}

int UnboundedStream::getSampleRate() const {
	return innerStream->getSampleRate();
}

int64_t UnboundedStream::getSampleCount() const {
	return innerStream->getSampleCount();
}

int64_t UnboundedStream::getSampleIndex() const {
	return sampleIndex;
}

void UnboundedStream::seek(int64_t sampleIndex) {
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
