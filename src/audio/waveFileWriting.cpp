#include <fstream>
#include "waveFileWriting.h"
#include "ioTools.h"

using namespace little_endian;

void createWaveFile(const AudioClip& audioClip, std::string fileName) {
	// Open file
	std::ofstream file;
	file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	file.open(fileName, std::ios::out | std::ios::binary);

	// Write RIFF chunk
	write<uint32_t>(fourcc('R', 'I', 'F', 'F'), file);
	uint32_t formatChunkSize = 16;
	uint16_t channelCount = 1;
	uint16_t frameSize = static_cast<uint16_t>(channelCount * sizeof(float));
	uint32_t dataChunkSize = static_cast<uint32_t>(audioClip.size() * frameSize);
	uint32_t riffChunkSize = 4 + (8 + formatChunkSize) + (8 + dataChunkSize);
	write<uint32_t>(riffChunkSize, file);
	write<uint32_t>(fourcc('W', 'A', 'V', 'E'), file);

	// Write format chunk
	write<uint32_t>(fourcc('f', 'm', 't', ' '), file);
	write<uint32_t>(formatChunkSize, file);
	uint16_t codec = 0x03; // 32-bit float
	write<uint16_t>(codec, file);
	write<uint16_t>(channelCount, file);
	uint32_t frameRate = static_cast<uint16_t>(audioClip.getSampleRate());
	write<uint32_t>(frameRate, file);
	uint32_t bytesPerSecond = frameRate * frameSize;
	write<uint32_t>(bytesPerSecond, file);
	write<uint16_t>(frameSize, file);
	uint16_t bitsPerSample = 8 * sizeof(float);
	write<uint16_t>(bitsPerSample, file);

	// Write data chunk
	write<uint32_t>(fourcc('d', 'a', 't', 'a'), file);
	write<uint32_t>(dataChunkSize, file);
	for (float sample : audioClip) {
		write<float>(sample, file);
	}
}
