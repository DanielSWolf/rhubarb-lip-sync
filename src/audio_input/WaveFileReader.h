#ifndef LIPSYNC_WAVFILEREADER_H
#define LIPSYNC_WAVFILEREADER_H

#include <string>
#include <cstdint>
#include <fstream>
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
	virtual int getFrameRate() override ;
	virtual int getFrameCount() override;
	virtual int getChannelCount() override;
	virtual bool getNextSample(float &sample) override;

private:
	boost::filesystem::ifstream file;
	SampleFormat sampleFormat;
	int frameRate;
	int frameCount;
	int channelCount;
	int remainingSamples;
};

#endif //LIPSYNC_WAVFILEREADER_H
