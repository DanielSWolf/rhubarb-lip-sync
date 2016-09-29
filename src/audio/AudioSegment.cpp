#include "AudioSegment.h"

using std::unique_ptr;
using std::make_unique;

AudioSegment::AudioSegment(std::unique_ptr<AudioClip> inputClip, const TimeRange& range) :
	inputClip(std::move(inputClip)),
	sampleOffset(static_cast<int64_t>(range.getStart().count()) * this->inputClip->getSampleRate() / 100),
	sampleCount(static_cast<int64_t>(range.getDuration().count()) * this->inputClip->getSampleRate() / 100)
{
	if (sampleOffset < 0 || sampleOffset + sampleCount > this->inputClip->size()) {
		throw std::invalid_argument("Segment extends beyond input clip.");
	}
}

unique_ptr<AudioClip> AudioSegment::clone() const {
	return make_unique<AudioSegment>(*this);
}

SampleReader AudioSegment::createUnsafeSampleReader() const {
	return [read = inputClip->createSampleReader(), sampleOffset = sampleOffset](size_type index) {
		return read(index + sampleOffset);
	};
}

AudioEffect segment(const TimeRange& range) {
	return [range](unique_ptr<AudioClip> inputClip) {
		return make_unique<AudioSegment>(std::move(inputClip), range);
	};
}
