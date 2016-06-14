#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include "AudioStream.h"

enum class SampleFormat {
	UInt8,
	Int16,
	Int24,
	Float32
};

class WaveFileReader : public AudioStream {
public:
	WaveFileReader(boost::filesystem::path filePath);
	WaveFileReader(const WaveFileReader& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override ;
	int64_t getSampleCount() const override;
	int64_t getSampleIndex() const override;
	void seek(int64_t sampleIndex) override;
	float readSample() override;

private:
	void openFile();

private:
	boost::filesystem::path filePath;
	boost::filesystem::ifstream file;
	int bytesPerSample;
	SampleFormat sampleFormat;
	int frameRate;
	int64_t frameCount;
	int channelCount;
	std::streampos dataOffset;
	int64_t frameIndex;
};
