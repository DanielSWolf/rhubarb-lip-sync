#pragma once

#include "AudioClip.h"
#include <filesystem>

class OggOpusFileReader : public AudioClip
{
public:
	OggOpusFileReader(const std::filesystem::path &filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override { return 48000; }
	size_type size() const override { return sampleCount; }

private:
	SampleReader createUnsafeSampleReader() const override;

	std::filesystem::path filePath;
	int channelCount;
	size_type sampleCount;
};
