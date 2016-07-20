#pragma once

#include <boost/filesystem/path.hpp>
#include "AudioClip.h"

enum class SampleFormat {
	UInt8,
	Int16,
	Int24,
	Float32
};

class WaveFileReader : public AudioClip {
public:
	WaveFileReader(boost::filesystem::path filePath);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;

private:
	SampleReader createUnsafeSampleReader() const override;

	struct WaveFormatInfo {
		int bytesPerFrame;
		SampleFormat sampleFormat;
		int frameRate;
		int64_t frameCount;
		int channelCount;
		std::streampos dataOffset;
	};

	boost::filesystem::path filePath;
	WaveFormatInfo formatInfo;
};

inline int WaveFileReader::getSampleRate() const {
	return formatInfo.frameRate;
}

inline AudioClip::size_type WaveFileReader::size() const {
	return formatInfo.frameCount;
}
