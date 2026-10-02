#include "OggOpusFileReader.h"

#include <opusfile.h>
#include "tools/tools.h"
#include <format.h>
#include "tools/fileTools.h"

using std::filesystem::path;
using std::vector;
using std::make_shared;
using std::ifstream;
using std::ios_base;

namespace {

std::string opusErrorToString(int errorCode) {
	switch (errorCode) {
		case OP_EREAD:
			return "An underlying read, seek, or tell operation failed.";
		case OP_EFAULT:
			return "Internal logic fault; indicates a bug or heap/stack corruption.";
		case OP_EIMPL:
			return "Feature not implemented.";
		case OP_EINVAL:
			return "Invalid argument, or the stream is not in a state to perform the operation.";
		case OP_ENOTFORMAT:
			return "The given file/data was not recognized as Ogg Opus data.";
		case OP_EBADHEADER:
			return "The file/data is apparently an Ogg Opus stream, but contains a corrupted or undecipherable header.";
		case OP_EVERSION:
			return "The ID header contains an unrecognized version number.";
		case OP_ENOTAUDIO:
			return "Packet is not an audio packet.";
		case OP_EBADPACKET:
			return "Error in packet.";
		case OP_EBADLINK:
			return "The given link exists in the Opus data stream, but is not decipherable due to garbage or corruption.";
		case OP_ENOSEEK:
			return "The given stream is not seekable.";
		case OP_EBADTIMESTAMP:
			return "The first or last granule position of a link failed basic validity checks.";
		default:
			return "An unexpected Opus error occurred.";
	}
}

template<typename T>
T throwOnError(T code) {
	if (code < 0) {
		const std::string message =
			fmt::format("{} (Opus error {})", opusErrorToString(static_cast<int>(code)), code);
		throw std::runtime_error(message);
	}
	return code;
}

int readCallback(void* dataSource, unsigned char* buffer, int byteCount) {
	ifstream& stream = *static_cast<ifstream*>(dataSource);
	stream.read(reinterpret_cast<char*>(buffer), byteCount);
	const std::streamsize bytesRead = stream.gcount();
	stream.clear(); // In case we read past EOF
	return static_cast<int>(bytesRead);
}

int seekCallback(void* dataSource, opus_int64 offset, int origin) {
	static const vector<ios_base::seekdir> seekDirections {
		ios_base::beg, ios_base::cur, ios_base::end
	};

	ifstream& stream = *static_cast<ifstream*>(dataSource);
	stream.seekg(offset, seekDirections.at(origin));
	if (stream.fail()) {
		stream.clear();
		return -1;
	}
	return 0;
}

opus_int64 tellCallback(void* dataSource) {
	ifstream& stream = *static_cast<ifstream*>(dataSource);
	return static_cast<opus_int64>(stream.tellg());
}

// RAII wrapper around OggOpusFile
class OggOpusFileHandle final {
public:
	OggOpusFileHandle(const path& filePath);

	OggOpusFileHandle(const OggOpusFileHandle&) = delete;
	OggOpusFileHandle& operator=(const OggOpusFileHandle&) = delete;

	OggOpusFile* get() {
		return opusFile;
	}

	~OggOpusFileHandle() {
		op_free(opusFile);
	}

private:
	ifstream stream;
	OggOpusFile* opusFile;
};

OggOpusFileHandle::OggOpusFileHandle(const path& filePath) :
	stream(openFile(filePath)),
	opusFile(nullptr)
{
	// Throw only on badbit, not on failbit.
	// opusfile expects read operations past the end of the file to
	// succeed, not to throw.
	stream.exceptions(ifstream::badbit);

	// opusfile normally uses the `FILE` API from the C standard library.
	// This doesn't handle Unicode paths on Windows.
	// Use wrapper functions around `ifstream` instead.
	const OpusFileCallbacks callbacks { readCallback, seekCallback, tellCallback, nullptr };
	int error = 0;
	opusFile = op_open_callbacks(&stream, &callbacks, nullptr, 0, &error);
	throwOnError(error);
}

} // namespace

OggOpusFileReader::OggOpusFileReader(const path& filePath) :
	filePath(filePath)
{
	OggOpusFileHandle file(filePath);

	sampleCount = throwOnError(op_pcm_total(file.get(), -1));
}

std::unique_ptr<AudioClip> OggOpusFileReader::clone() const {
	return std::make_unique<OggOpusFileReader>(*this);
}

SampleReader OggOpusFileReader::createUnsafeSampleReader() const {
	// Opus streams may contain multiple links with different channel counts.
	// Always decode to stereo, then downmix that to mono.
	constexpr int channelCount = 2;
	// Opus packets are at most 120 ms long
	constexpr int maxSize = sampleRate * 120 / 1000;

	return [
		file = make_shared<OggOpusFileHandle>(filePath),
		buffer = vector<value_type>(maxSize * channelCount),
		bufferStart = size_type(0),
		bufferSize = size_type(0)
	](size_type index) mutable {
		if (index < bufferStart || index >= bufferStart + bufferSize) {
			// Seek, unless we're reading sequentially.
			// Seeking is expensive with Opus, as it requires decoding a pre-roll.
			if (index != bufferStart + bufferSize) {
				throwOnError(op_pcm_seek(file->get(), index));
			}

			// Read a block of samples
			int samplesRead;
			do {
				// OP_HOLE, though technically an error code, is only informational
				samplesRead = op_read_float_stereo(
					file->get(), buffer.data(), static_cast<int>(buffer.size())
				);
			} while (samplesRead == OP_HOLE);
			bufferStart = index;
			bufferSize = throwOnError(samplesRead);
			if (bufferSize == 0) {
				throw std::runtime_error("Unexpected end of file.");
			}
		}

		// Downmix channels
		const size_type bufferIndex = index - bufferStart;
		value_type sum = 0.0f;
		for (int channel = 0; channel < channelCount; ++channel) {
			sum += buffer[bufferIndex * channelCount + channel];
		}
		return sum / channelCount;
	};
}
