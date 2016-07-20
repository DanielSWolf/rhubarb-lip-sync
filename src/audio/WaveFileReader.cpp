#include <format.h>
#include <string.h>
#include "WaveFileReader.h"
#include "ioTools.h"

using std::runtime_error;
using fmt::format;
using std::string;
using namespace little_endian;
using std::unique_ptr;
using std::make_unique;
using std::make_shared;
using boost::filesystem::path;

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

std::ifstream openFile(path filePath) {
	try {
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(filePath.c_str(), std::ios::binary);

		// Error messages on stream exceptions are mostly useless.
		// Read some dummy data so that we can throw a decent exception in case the file is missing, locked, etc.
		file.seekg(0, std::ios_base::end);
		if (file.tellg()) {
			file.seekg(0);
			file.get();
			file.seekg(0);
		}

		return std::move(file);
	} catch (const std::ifstream::failure&) {
		char message[256];
		strerror_s(message, sizeof message, errno);
		throw runtime_error(message);
	}
}

WaveFileReader::WaveFileReader(path filePath) :
	filePath(filePath),
	formatInfo{}
{
	auto file = openFile(filePath);

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
	while (!reachedDataChunk && remaining(8)) {
		uint32_t chunkId = read<uint32_t>(file);
		int chunkSize = read<uint32_t>(file);
		switch (chunkId) {
		case fourcc('f', 'm', 't', ' '): {
			// Read relevant data
			Codec codec = static_cast<Codec>(read<uint16_t>(file));
			formatInfo.channelCount = read<uint16_t>(file);
			formatInfo.frameRate = read<uint32_t>(file);
			read<uint32_t>(file); // Bytes per second
			int frameSize = read<uint16_t>(file);
			int bitsPerSample = read<uint16_t>(file);

			// We've read 16 bytes so far. Skip the remainder.
			file.seekg(roundToEven(chunkSize) - 16, file.cur);

			// Determine sample format
			int bytesPerSample;
			switch (codec) {
			case Codec::PCM:
				// Determine sample size.
				// According to the WAVE standard, sample sizes that are not multiples of 8 bits
				// (e.g. 12 bits) can be treated like the next-larger byte size.
				if (bitsPerSample == 8) {
					formatInfo.sampleFormat = SampleFormat::UInt8;
					bytesPerSample = 1;
				} else if (bitsPerSample <= 16) {
					formatInfo.sampleFormat = SampleFormat::Int16;
					bytesPerSample = 2;
				} else if (bitsPerSample <= 24) {
					formatInfo.sampleFormat = SampleFormat::Int24;
					bytesPerSample = 3;
				} else {
					throw runtime_error(
						format("Unsupported sample format: {}-bit integer samples.", bitsPerSample));
				}
				if (bytesPerSample != frameSize / formatInfo.channelCount) {
					throw runtime_error("Unsupported sample organization.");
				}
				break;
			case Codec::Float:
				if (bitsPerSample == 32) {
					formatInfo.sampleFormat = SampleFormat::Float32;
					bytesPerSample = 4;
				} else {
					throw runtime_error(format("Unsupported sample format: {}-bit floating-point samples.", bitsPerSample));
				}
				break;
			default:
				throw runtime_error("Unsupported sample format. Only uncompressed formats are supported.");
			}
			formatInfo.bytesPerFrame = bytesPerSample * formatInfo.channelCount;
			break;
		}
		case fourcc('d', 'a', 't', 'a'): {
			reachedDataChunk = true;
			formatInfo.dataOffset = file.tellg();
			formatInfo.frameCount = chunkSize / formatInfo.bytesPerFrame;
			break;
		}
		default: {
			// Skip unknown chunk
			file.seekg(roundToEven(chunkSize), file.cur);
			break;
		}
		}
	}
}

unique_ptr<AudioClip> WaveFileReader::clone() const {
	return make_unique<WaveFileReader>(*this);
}

inline AudioClip::value_type readSample(std::ifstream& file, SampleFormat sampleFormat, int channelCount) {
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

SampleReader WaveFileReader::createUnsafeSampleReader() const {
	return [formatInfo = formatInfo, file = std::make_shared<std::ifstream>(openFile(filePath)), filePos = std::streampos(0)](size_type index) mutable {
		std::streampos newFilePos = formatInfo.dataOffset + static_cast<std::streamoff>(index * formatInfo.bytesPerFrame);
		file->seekg(newFilePos);
		value_type result = readSample(*file, formatInfo.sampleFormat, formatInfo.channelCount);
		filePos = newFilePos + static_cast<std::streamoff>(formatInfo.bytesPerFrame);
		return result;
	};
}
