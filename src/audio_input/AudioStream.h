#ifndef LIPSYNC_AUDIOSTREAM_H
#define LIPSYNC_AUDIOSTREAM_H

class AudioStream {
public:
	virtual int getFrameRate() = 0;
	virtual int getFrameCount() = 0;
	virtual int getChannelCount() = 0;
	virtual bool getNextSample(float &sample) = 0;
};

#endif //LIPSYNC_AUDIOSTREAM_H
