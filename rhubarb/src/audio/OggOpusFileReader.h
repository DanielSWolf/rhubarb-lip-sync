#pragma once

#include "AudioClip.h"
#include <filesystem>

class OggOpusFileReader : public AudioClip {
public:
	OggOpusFileReader(const std::filesystem::path& filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override { return sampleRate; }
	size_type size() const override { return sampleCount; }

private:
	SampleReader createUnsafeSampleReader() const override;

	// opusfile always decodes to 48 kHz, regardless of the original sample rate
	static constexpr int sampleRate = 48000;

	std::filesystem::path filePath;
	size_type sampleCount;
};
