#pragma once

#include "AudioClip.h"
#include <boost/filesystem/path.hpp>

class OggVorbisFileReader : public AudioClip {
public:
	OggVorbisFileReader(const boost::filesystem::path& filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override { return sampleRate; }
	size_type size() const override { return sampleCount; }

private:
	SampleReader createUnsafeSampleReader() const override;

	boost::filesystem::path filePath;
	int sampleRate;
	int channelCount;
	size_type sampleCount;
};
