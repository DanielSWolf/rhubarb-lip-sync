#ifndef LIPSYNC_WAVFILEREADER_H
#define LIPSYNC_WAVFILEREADER_H

#include <string>
#include <cstdint>
#include <fstream>
#include "AudioStream.h"

enum class SampleFormat {
	UInt8,
	Int16,
	Int24,
	Float32
};

class WaveFileReader : public AudioStream {
public:
	WaveFileReader(std::string fileName);
	virtual int getFrameRate() override ;
	virtual int getFrameCount() override;
	virtual int getChannelCount() override;
	virtual bool getNextSample(float &sample) override;

private:
	std::ifstream file;
	SampleFormat sampleFormat;
	int frameRate;
	int frameCount;
	int channelCount;
	int remainingSamples;
};

#endif //LIPSYNC_WAVFILEREADER_H
