#include "OggOpusFileReader.h"

#include <opusfile.h>
#include "tools/fileTools.h"
#include <format.h>

using std::filesystem::path;
using std::make_shared;
using std::ifstream;
using std::ios_base;

// Opus always decodes to 48 kHz internally.
static constexpr int opusSampleRate = 48000;

static std::string opusErrorToString(int errorCode) {
	switch (errorCode) {
		case OP_EREAD:      return "Read, seek, or tell failed.";
		case OP_EFAULT:     return "Internal logic fault; indicates a bug or heap/stack corruption.";
		case OP_EIMPL:      return "Feature not implemented.";
		case OP_EINVAL:     return "Invalid argument.";
		case OP_ENOTFORMAT: return "The file is not in Ogg Opus format.";
		case OP_EBADHEADER: return "The file contains a corrupted or undecipherable header.";
		case OP_EVERSION:   return "The ID header has an unrecognized version number.";
		case OP_EBADPACKET: return "An audio packet failed to decode properly.";
		case OP_EBADLINK:   return "We failed to find data in the stream.";
		case OP_ENOSEEK:    return "The stream is not seekable.";
		case OP_EBADTIMESTAMP: return "The stream contains a bad timestamp.";
		default:            return "An unexpected Opus error occurred.";
	}
}

template<typename T>
T throwOnOpusError(T code) {
	if (code < 0) {
		throw std::runtime_error(
			fmt::format("{} (Opus error {})", opusErrorToString(static_cast<int>(code)), static_cast<int>(code)));
	}
	return code;
}

// ifstream-backed I/O callbacks — the same Unicode-safe approach used by
// OggVorbisFileReader.  Using C-library FILE* would silently fail on
// Windows paths that contain non-ASCII characters.
static int opusReadCallback(void* dataSource, unsigned char* buffer, int bytes) {
	ifstream& stream = *static_cast<ifstream*>(dataSource);
	stream.read(reinterpret_cast<char*>(buffer), bytes);
	const std::streamsize bytesRead = stream.gcount();
	stream.clear(); // clear EOF so further seeks work
	return static_cast<int>(bytesRead);
}

static int opusSeekCallback(void* dataSource, opus_int64 offset, int whence) {
	static const std::vector<ios_base::seekdir> seekDirections{
		ios_base::beg, ios_base::cur, ios_base::end
	};
	ifstream& stream = *static_cast<ifstream*>(dataSource);
	stream.seekg(static_cast<std::streamoff>(offset), seekDirections.at(whence));
	stream.clear();
	return 0;
}

static opus_int64 opusTellCallback(void* dataSource) {
	ifstream& stream = *static_cast<ifstream*>(dataSource);
	return static_cast<opus_int64>(stream.tellg());
}

// RAII wrapper around OggOpusFile
class OggOpusFile final {
public:
	explicit OggOpusFile(const path& filePath);

	OggOpusFile(const OggOpusFile&) = delete;
	OggOpusFile& operator=(const OggOpusFile&) = delete;

	::OggOpusFile* get() { return handle; }

	~OggOpusFile() {
		if (handle) op_free(handle);
	}

private:
	::OggOpusFile* handle;
	ifstream stream;
};

OggOpusFile::OggOpusFile(const path& filePath) :
	handle(nullptr),
	stream(openFile(filePath))
{
	stream.exceptions(ifstream::badbit);

	static const OpusFileCallbacks callbacks{
		opusReadCallback,
		opusSeekCallback,
		opusTellCallback,
		nullptr // no close callback — the ifstream owns the file
	};

	int error = 0;
	handle = op_open_callbacks(&stream, &callbacks, nullptr, 0, &error);
	if (!handle) {
		throw std::runtime_error(
			fmt::format("{} (Opus error {})", opusErrorToString(error), error));
	}
}

OggOpusFileReader::OggOpusFileReader(const path& filePath) :
	filePath(filePath)
{
	OggOpusFile file(filePath);

	// op_channel_count(-1) queries the first (and typically only) link.
	channelCount = op_channel_count(file.get(), -1);
	sampleRate = opusSampleRate;

	// op_pcm_total returns the total number of PCM samples per channel.
	sampleCount = throwOnOpusError(op_pcm_total(file.get(), -1));
}

std::unique_ptr<AudioClip> OggOpusFileReader::clone() const {
	return std::make_unique<OggOpusFileReader>(*this);
}

SampleReader OggOpusFileReader::createUnsafeSampleReader() const {
	// op_read_float_stereo always returns interleaved stereo (2 channels).
	// For mono sources opusfile up-mixes automatically.
	constexpr int stereoChannels = 2;
	constexpr int maxFramesPerRead = 960; // one Opus frame at 48 kHz
	const int bufferSize = maxFramesPerRead * stereoChannels;

	return [
		file = make_shared<OggOpusFile>(filePath),
		buffer = std::vector<float>(bufferSize),
		bufferStart = size_type(0),
		bufferFrames = size_type(0)
	](size_type index) mutable -> value_type {
		if (index < bufferStart || index >= bufferStart + bufferFrames) {
			// Seek to the requested sample position
			throwOnOpusError(op_pcm_seek(file->get(), static_cast<ogg_int64_t>(index)));

			// Read the next block of interleaved stereo samples
			const int framesRead = throwOnOpusError(
				op_read_float_stereo(file->get(), buffer.data(), static_cast<int>(buffer.size())));
			if (framesRead == 0) {
				throw std::runtime_error("Unexpected end of Opus file.");
			}
			bufferStart  = index;
			bufferFrames = static_cast<size_type>(framesRead);
		}

		// Average the two stereo channels to produce a mono sample
		const size_type frameIndex = index - bufferStart;
		return (buffer[frameIndex * stereoChannels] + buffer[frameIndex * stereoChannels + 1]) / 2.0f;
	};
}
