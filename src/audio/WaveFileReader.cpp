#include <format.h>
#include <string.h>
#include "WaveFileReader.h"
#include "ioTools.h"

using std::runtime_error;
using fmt::format;
using std::string;
using namespace little_endian;

#define INT24_MIN (-8388608)
#define INT24_MAX 8388607

// Converts an int in the range min..max to a float in the range -1..1
float toNormalizedFloat(int value, int min, int max) {
	return (static_cast<float>(value - min) / (max - min) * 2) - 1;
}

int roundToEven(int i) {
	return (i + 1) & (~1);
}

enum class Codec {
	PCM = 0x01,
	Float = 0x03
};

WaveFileReader::WaveFileReader(boost::filesystem::path filePath) :
	filePath(filePath),
	file(),
	sampleIndex(0)
{
	openFile();

	file.seekg(0, std::ios_base::end);
	std::streamoff fileSize = file.tellg();
	file.seekg(0);

	auto remaining = [&](int byteCount) {
		std::streamoff filePosition = file.tellg();
		return byteCount <= fileSize - filePosition;
	};

	// Read header
	if (!remaining(10)) {
		throw runtime_error("WAVE file is corrupt. Header not found.");
	}
	uint32_t rootChunkId = read<uint32_t>(file);
	if (rootChunkId != fourcc('R', 'I', 'F', 'F')) {
		throw runtime_error("Unknown file format. Only WAVE files are supported.");
	}
	read<uint32_t>(file); // Chunk size
	uint32_t waveId = read<uint32_t>(file);
	if (waveId != fourcc('W', 'A', 'V', 'E')) {
		throw runtime_error(format("File format is not WAVE, but {}.", fourccToString(waveId)));
	}

	// Read chunks until we reach the data chunk
	bool reachedDataChunk = false;
	bytesPerSample = 0;
	while (!reachedDataChunk && remaining(8)) {
		uint32_t chunkId = read<uint32_t>(file);
		int chunkSize = read<uint32_t>(file);
		switch (chunkId) {
		case fourcc('f', 'm', 't', ' '): {
			// Read relevant data
			Codec codec = (Codec)read<uint16_t>(file);
			channelCount = read<uint16_t>(file);
			frameRate = read<uint32_t>(file);
			read<uint32_t>(file); // Bytes per second
			int frameSize = read<uint16_t>(file);
			int bitsPerSample = read<uint16_t>(file);

			// We've read 16 bytes so far. Skip the remainder.
			file.seekg(roundToEven(chunkSize) - 16, file.cur);

			// Determine sample format
			switch (codec) {
			case Codec::PCM:
				// Determine sample size.
				// According to the WAVE standard, sample sizes that are not multiples of 8 bits
				// (e.g. 12 bits) can be treated like the next-larger byte size.
				if (bitsPerSample == 8) {
					sampleFormat = SampleFormat::UInt8;
					bytesPerSample = 1;
				} else if (bitsPerSample <= 16) {
					sampleFormat = SampleFormat::Int16;
					bytesPerSample = 2;
				} else if (bitsPerSample <= 24) {
					sampleFormat = SampleFormat::Int24;
					bytesPerSample = 3;
				} else {
					throw runtime_error(
						format("Unsupported sample format: {}-bit integer samples.", bitsPerSample));
				}
				if (bytesPerSample != frameSize / channelCount) {
					throw runtime_error("Unsupported sample organization.");
				}
				break;
			case Codec::Float:
				if (bitsPerSample == 32) {
					sampleFormat = SampleFormat::Float32;
					bytesPerSample = 4;
				} else {
					throw runtime_error(format("Unsupported sample format: {}-bit floating-point samples.", bitsPerSample));
				}
				break;
			default:
				throw runtime_error("Unsupported sample format. Only uncompressed formats are supported.");
			}
			break;
		}
		case fourcc('d', 'a', 't', 'a'): {
			reachedDataChunk = true;
			dataOffset = file.tellg();
			sampleCount = chunkSize / bytesPerSample;
			frameCount = sampleCount / channelCount;
			break;
		}
		default: {
			// Skip unknown chunk
			file.seekg(roundToEven(chunkSize), file.cur);
			break;
		}
		}
	}

	if (!reachedDataChunk) {
		dataOffset = file.tellg();
		sampleCount = frameCount = 0;
	}
}

WaveFileReader::WaveFileReader(const WaveFileReader& rhs, bool reset) :
	filePath(rhs.filePath),
	file(),
	bytesPerSample(rhs.bytesPerSample),
	sampleFormat(rhs.sampleFormat),
	frameRate(rhs.frameRate),
	frameCount(rhs.frameCount),
	channelCount(rhs.channelCount),
	sampleCount(rhs.sampleCount),
	dataOffset(rhs.dataOffset),
	sampleIndex(-1)
{
	openFile();
	seek(reset ? 0 : rhs.sampleIndex);
}

std::unique_ptr<AudioStream> WaveFileReader::clone(bool reset) const {
	return std::make_unique<WaveFileReader>(*this, reset);
}

void WaveFileReader::openFile() {
	try {
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(filePath, std::ios::binary);

		// Error messages on stream exceptions are mostly useless.
		// Read some dummy data so that we can throw a decent exception in case the file is missing, locked, etc.
		file.seekg(0, std::ios_base::end);
		if (file.tellg()) {
			file.seekg(0);
			file.get();
			file.seekg(0);
		}
	} catch (const std::ifstream::failure&) {
		throw runtime_error(strerror(errno));
	}
}

int WaveFileReader::getSampleRate() const {
	return frameRate;
}

int64_t WaveFileReader::getSampleCount() const {
	return frameCount;
}

int64_t WaveFileReader::getSampleIndex() const {
	return sampleIndex;
}

void WaveFileReader::seek(int64_t sampleIndex) {
	if (sampleIndex < 0 || sampleIndex > sampleCount) throw std::invalid_argument("sampleIndex out of range.");

	file.seekg(dataOffset + static_cast<std::streamoff>(sampleIndex * channelCount * bytesPerSample));
	this->sampleIndex = sampleIndex;
}

float WaveFileReader::readSample() {
	if (sampleIndex + channelCount > sampleCount) throw std::out_of_range("End of stream.");
	sampleIndex += channelCount;

	float sum = 0;
	for (int channelIndex = 0; channelIndex < channelCount; channelIndex++) {
		switch (sampleFormat) {
		case SampleFormat::UInt8: {
			uint8_t raw = read<uint8_t>(file);
			sum += toNormalizedFloat(raw, 0, UINT8_MAX);
			break;
		}
		case SampleFormat::Int16: {
			int16_t raw = read<int16_t>(file);
			sum += toNormalizedFloat(raw, INT16_MIN, INT16_MAX);
			break;
		}
		case SampleFormat::Int24: {
			int raw = read<int, 24>(file);
			if (raw & 0x800000) raw |= 0xFF000000; // Fix two's complement
			sum += toNormalizedFloat(raw, INT24_MIN, INT24_MAX);
			break;
		}
		case SampleFormat::Float32: {
			sum += read<float>(file);
			break;
		}
		}
	}

	return sum / channelCount;
}
