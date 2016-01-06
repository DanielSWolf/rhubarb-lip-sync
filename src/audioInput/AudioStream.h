#pragma once

class AudioStream {
public:
	virtual int getFrameRate() = 0;
	virtual int getFrameCount() = 0;
	virtual int getChannelCount() = 0;
	virtual bool getNextSample(float &sample) = 0;
};
