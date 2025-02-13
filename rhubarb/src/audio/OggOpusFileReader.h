#pragma once

#include "AudioClip.h"
#include <filesystem>

class OggOpusFileReader : public AudioClip
{
public:
	OggOpusFileReader(const std::filesystem::path &filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override { return SAMPLE_RATE; }
	size_type size() const override { return sampleCount; }

private:
	SampleReader createUnsafeSampleReader() const override;

	std::filesystem::path filePath;
	static constexpr int SAMPLE_RATE = 48000; 
	int channelCount;
	size_type sampleCount;
};
