#ifndef LIPSYNC_CHANNELDOWNMIXER_H
#define LIPSYNC_CHANNELDOWNMIXER_H

#include "AudioStream.h"
#include <memory>

// Converts a multi-channel audio stream to mono.
class ChannelDownmixer : public AudioStream {
public:
	ChannelDownmixer(std::unique_ptr<AudioStream> inputStream);
	virtual int getFrameRate() override;
	virtual int getFrameCount() override;
	virtual int getChannelCount() override;
	virtual bool getNextSample(float &sample) override;

private:
	std::unique_ptr<AudioStream> inputStream;
	int inputChannelCount;
};

#endif //LIPSYNC_CHANNELDOWNMIXER_H
