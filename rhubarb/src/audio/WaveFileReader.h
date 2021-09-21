#pragma once

#include <filesystem>
#include "AudioClip.h"

enum class SampleFormat {
	UInt8,
	Int16,
	Int24,
	Int32,
	Float32,
	Float64
};

struct WaveFormatInfo {
	int bytesPerFrame;
	SampleFormat sampleFormat;
	int frameRate;
	int64_t frameCount;
	int channelCount;
	std::streampos dataOffset;
};

WaveFormatInfo getWaveFormatInfo(const std::filesystem::path& filePath);

class WaveFileReader : public AudioClip {
public:
	WaveFileReader(const std::filesystem::path& filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;

private:
	SampleReader createUnsafeSampleReader() const override;

	std::filesystem::path filePath;
	WaveFormatInfo formatInfo;
};

inline int WaveFileReader::getSampleRate() const {
	return formatInfo.frameRate;
}

inline AudioClip::size_type WaveFileReader::size() const {
	return formatInfo.frameCount;
}
