#include "AudioClip.h"
#include <format.h>

using std::invalid_argument;

TimeRange AudioClip::getTruncatedRange() const {
	return TimeRange(0_cs, centiseconds(100 * size() / getSampleRate()));
}

class SafeSampleReader {
public:
	SafeSampleReader(SampleReader unsafeRead, AudioClip::size_type size);
	AudioClip::value_type operator()(AudioClip::size_type index);
private:
	SampleReader unsafeRead;
	AudioClip::size_type size;
	AudioClip::size_type lastIndex = -1;
	AudioClip::value_type lastSample = 0;
};

SafeSampleReader::SafeSampleReader(SampleReader unsafeRead, AudioClip::size_type size) :
	unsafeRead(unsafeRead),
	size(size)
{}

inline AudioClip::value_type SafeSampleReader::operator()(AudioClip::size_type index) {
	if (index < 0) {
		throw invalid_argument(fmt::format("Cannot read from sample index {}. Index < 0.", index));
	}
	if (index >= size) {
		throw invalid_argument(fmt::format("Cannot read from sample index {}. Clip size is {}.", index, size));
	}
	if (index == lastIndex) {
		return lastSample;
	}

	lastIndex = index;
	lastSample = unsafeRead(index);
	return lastSample;
}

SampleReader AudioClip::createSampleReader() const {
	return SafeSampleReader(createUnsafeSampleReader(), size());
}

AudioClip::iterator AudioClip::begin() const {
	return SampleIterator(*this, 0);
}

AudioClip::iterator AudioClip::end() const {
	return SampleIterator(*this, size());
}

std::unique_ptr<AudioClip> operator|(std::unique_ptr<AudioClip> clip, AudioEffect effect) {
	return effect(std::move(clip));
}

SampleIterator::SampleIterator() :
	sampleIndex(0)
{}

SampleIterator::SampleIterator(const AudioClip& audioClip, size_type sampleIndex) :
	sampleReader([&audioClip] { return audioClip.createSampleReader(); }),
	sampleIndex(sampleIndex)
{}
