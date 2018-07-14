#include "OggVorbisFileReader.h"

#include <stdlib.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include "tools/tools.h"
#include <format.h>
#include <numeric>
#include "tools/fileTools.h"

using boost::filesystem::path;
using std::vector;
using std::make_shared;

std::string vorbisErrorToString(int64_t errorCode) {
	switch (errorCode) {
	case OV_EREAD:
		return "Read error while fetching compressed data for decode.";
	case OV_EFAULT:
		return "Internal logic fault; indicates a bug or heap/stack corruption.";
	case OV_EIMPL:
		return "Feature not implemented";
	case OV_EINVAL:
		return "Either an invalid argument, or incompletely initialized argument passed to a call.";
	case OV_ENOTVORBIS:
		return "The given file/data was not recognized as Ogg Vorbis data.";
	case OV_EBADHEADER:
		return "The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header.";
	case OV_EVERSION:
		return "The bitstream format revision of the given Vorbis stream is not supported.";
	case OV_ENOTAUDIO:
		return "Packet is not an audio packet.";
	case OV_EBADPACKET:
		return "Error in packet.";
	case OV_EBADLINK:
		return "The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption.";
	case OV_ENOSEEK:
		return "The given stream is not seekable.";
	default:
		return "An unexpected Vorbis error occurred.";
	}
}

template<typename T>
T throwOnError(T code) {
	// OV_HOLE, though technically an error code, is only informational
	const bool error = code < 0 && code != OV_HOLE;
	if (error) {
		const std::string message =
			fmt::format("{} (Vorbis error {})", vorbisErrorToString(code), code);
		throw std::runtime_error(message);
	}
	return code;
}

// RAII wrapper around OggVorbis_File
class OggVorbisFile {
public:
	OggVorbisFile(const OggVorbisFile&) = delete;
	OggVorbisFile& operator=(const OggVorbisFile&) = delete;

	OggVorbisFile(const path& filePath) {
		throwOnError(ov_fopen(filePath.string().c_str(), &file));
	}

	OggVorbis_File* get() {
		return &file;
	}

	~OggVorbisFile() {
		ov_clear(&file);
	}

private:
	OggVorbis_File file;
};

OggVorbisFileReader::OggVorbisFileReader(const path& filePath) :
	filePath(filePath)
{
	// Make sure that common error cases result in readable exception messages
	throwIfNotReadable(filePath);

	OggVorbisFile file(filePath);
	
	vorbis_info* vorbisInfo = ov_info(file.get(), -1);
	sampleRate = vorbisInfo->rate;
	channelCount = vorbisInfo->channels;
	
	sampleCount = throwOnError(ov_pcm_total(file.get(), -1));
}

std::unique_ptr<AudioClip> OggVorbisFileReader::clone() const {
	return std::make_unique<OggVorbisFileReader>(*this);
}

SampleReader OggVorbisFileReader::createUnsafeSampleReader() const {
	return [
		channelCount = channelCount,
		file = make_shared<OggVorbisFile>(filePath),
		buffer = static_cast<value_type**>(nullptr),
		bufferStart = size_type(0),
		bufferSize = size_type(0)
	](size_type index) mutable {
		if (index < bufferStart || index >= bufferStart + bufferSize) {
			// Seek
			throwOnError(ov_pcm_seek(file->get(), index));

			// Read a block of samples
			constexpr int maxSize = 1024;
			bufferStart = index;
			bufferSize = throwOnError(ov_read_float(file->get(), &buffer, maxSize, nullptr));
			if (bufferSize == 0) {
				throw std::runtime_error("Unexpected end of file.");
			}
		}

		// Downmix channels
		size_type bufferIndex = index - bufferStart;
		value_type sum = 0.0f;
		for (int channel = 0; channel < channelCount; ++channel) {
			sum += buffer[channel][bufferIndex];
		}
		return sum / channelCount;
	};
}
