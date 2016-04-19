#pragma once
#include <audio/AudioStream.h>
#include <TimeRange.h>

class AudioStreamSegment : public AudioStream {
public:
	AudioStreamSegment(std::unique_ptr<AudioStream> audioStream, const TimeRange& range);
	AudioStreamSegment(const AudioStreamSegment& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override;
	int getSampleCount() const override;
	int getSampleIndex() const override;
	void seek(int sampleIndex) override;
	float readSample() override;

private:
	std::unique_ptr<AudioStream> audioStream;
	int sampleOffset, sampleCount;
};

std::unique_ptr<AudioStream> createSegment(std::unique_ptr<AudioStream> audioStream, const TimeRange& range);
