#include "OggVorbisFileReader.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include "tools/tools.h"
#include <format.h>
#include "tools/fileTools.h"

using std::filesystem::path;
using std::vector;
using std::make_shared;
using std::ifstream;
using std::ios_base;

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
            return "The given link exists in the Vorbis data stream, but is not decipherable due to garbage or corruption.";
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

size_t readCallback(void* buffer, size_t elementSize, size_t elementCount, void* dataSource) {
    assert(elementSize == 1);

    ifstream& stream = *static_cast<ifstream*>(dataSource);
    stream.read(static_cast<char*>(buffer), elementCount);
    const std::streamsize bytesRead = stream.gcount();
    stream.clear(); // In case we read past EOF
    return static_cast<size_t>(bytesRead);
}

int seekCallback(void* dataSource, ogg_int64_t offset, int origin) {
    static const vector<ios_base::seekdir> seekDirections {
        ios_base::beg, ios_base::cur, ios_base::end
    };

    ifstream& stream = *static_cast<ifstream*>(dataSource);
    stream.seekg(offset, seekDirections.at(origin));
    stream.clear(); // In case we sought to EOF
    return 0;
}

long tellCallback(void* dataSource) {
    ifstream& stream = *static_cast<ifstream*>(dataSource);
    const auto position = stream.tellg();
    assert(position >= 0);
    return static_cast<long>(position);
}

// RAII wrapper around OggVorbis_File
class OggVorbisFile final {
public:
    OggVorbisFile(const path& filePath);

    OggVorbisFile(const OggVorbisFile&) = delete;
    OggVorbisFile& operator=(const OggVorbisFile&) = delete;

    OggVorbis_File* get() {
        return &oggVorbisHandle;
    }

    ~OggVorbisFile() {
        ov_clear(&oggVorbisHandle);
    }

private:
    OggVorbis_File oggVorbisHandle;
    ifstream stream;
};

OggVorbisFile::OggVorbisFile(const path& filePath) :
    oggVorbisHandle(),
    stream(openFile(filePath))
{
    // Throw only on badbit, not on failbit.
    // Ogg Vorbis expects read operations past the end of the file to
    // succeed, not to throw.
    stream.exceptions(ifstream::badbit);

    // Ogg Vorbis normally uses the `FILE` API from the C standard library.
    // This doesn't handle Unicode paths on Windows.
    // Use wrapper functions around `ifstream` instead.
    const ov_callbacks callbacks { readCallback, seekCallback, nullptr, tellCallback };
    throwOnError(ov_open_callbacks(&stream, &oggVorbisHandle, nullptr, 0, callbacks));
}

OggVorbisFileReader::OggVorbisFileReader(const path& filePath) :
    filePath(filePath)
{
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
        const size_type bufferIndex = index - bufferStart;
        value_type sum = 0.0f;
        for (int channel = 0; channel < channelCount; ++channel) {
            sum += buffer[channel][bufferIndex];
        }
        return sum / channelCount;
    };
}
