#include "OggOpusFileReader.h"
#include "tools/fileTools.h"
#include "tools/tools.h"
#include <format.h>
#include <fstream>
#include <opusfile.h>
#include <stdexcept>

using std::ifstream;
using std::ios_base;
using std::make_shared;
using std::filesystem::path;

std::string opusErrorToString(int errorCode) {
	switch (errorCode) {
	case OP_EREAD:
		return "A read from media returned an error.";
	case OP_EFAULT:
		return "An internal logic fault; indicates a bug or heap/stack corruption.";
	case OP_EIMPL:
		return "Feature not implemented.";
	case OP_EINVAL:
		return "Invalid argument.";
	case OP_ENOTFORMAT:
		return "Not a recognized format.";
	case OP_EBADHEADER:
		return "Invalid header.";
	case OP_EVERSION:
		return "Unsupported version.";
	case OP_ENOTAUDIO:
		return "Not audio data.";
	case OP_EBADPACKET:
		return "Bad packet.";
	case OP_EBADLINK:
		return "Bad link.";
	case OP_ENOSEEK:
		return "Stream is not seekable.";
	default:
		return "An unexpected Opus error occurred.";
	}
}

template <typename T>
T throwOnError(T code) {
        // OP_HOLE, though technically an error code, is only informational
	if (code < 0 && code != OP_HOLE)
	{
		const std::string message = fmt::format("{} (Opus error {})", opusErrorToString(code), code);
		throw std::runtime_error(message);
	}
	return code;
}

// RAII wrapper around OggOpusFile
class OggOpusFileHandle final {
public:
	OggOpusFileHandle(const path& filePath);

	OggOpusFileHandle(const OggOpusFileHandle&) = delete;
	OggOpusFileHandle& operator=(const OggOpusFileHandle&) = delete;

	OggOpusFile* get()
	{
		return opusFile;
	}

	~OggOpusFileHandle()
	{
		if (opusFile) {
			op_free(opusFile);
		}
	}

private:
	OggOpusFile* opusFile;
};

OggOpusFileHandle::OggOpusFileHandle(const path& filePath)
	: opusFile(nullptr)
{
	int error;
	opusFile = op_open_file(filePath.string().c_str(), &error);
	throwOnError(error);
}

OggOpusFileReader::OggOpusFileReader(const path& filePath)
	: filePath(filePath)
{
	OggOpusFileHandle file(filePath);

	const OpusHead* opusHead = op_head(file.get(), -1);
	if (!opusHead) {
		throw std::runtime_error("Failed to retrieve Opus header (null pointer).");
	}
	channelCount = opusHead->channel_count;

	ogg_int64_t pcmTotal = op_pcm_total(file.get(), -1);
	if (pcmTotal < 0) {
		throw std::runtime_error("Failed to get total PCM samples.");
	}
	sampleCount = static_cast<size_type>(pcmTotal);
}

std::unique_ptr<AudioClip> OggOpusFileReader::clone() const {
	return std::make_unique<OggOpusFileReader>(*this);
}

SampleReader OggOpusFileReader::createUnsafeSampleReader() const {
	constexpr int maxSize = 1024;

	return [
		channelCount = channelCount,
		file = make_shared<OggOpusFileHandle>(filePath),
		buffer = std::vector<float>(maxSize * channelCount),
		bufferStart = size_type(0),
		bufferSize = size_type(0)
	](size_type index) mutable {
		if (index < bufferStart || index >= bufferStart + bufferSize) {
			// Seek
			throwOnError(op_pcm_seek(file->get(), index));

			bufferStart = index;
			int samplesRead = throwOnError(op_read_float(file->get(), buffer.data(), maxSize, nullptr));
			bufferSize = static_cast<size_type>(samplesRead);
			if (bufferSize == 0) {
				throw std::runtime_error("Unexpected end of file.");
			}
		}

		// Downmix channels
		const size_type bufferIndex = index - bufferStart;
		float sum = 0.0f;
		for (int channel = 0; channel < channelCount; ++channel) {
			sum += buffer[bufferIndex * channelCount + channel];
		}
		return sum / channelCount;
	};
}
