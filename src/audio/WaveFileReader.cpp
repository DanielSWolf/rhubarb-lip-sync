#include <format.h>
#include <string.h>
#include "WaveFileReader.h"
#include "ioTools.h"
#include "tools/platformTools.h"

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

namespace Codec {
	constexpr int Pcm = 0x01;
	constexpr int Float = 0x03;
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
		throw runtime_error(errorNumberToString(errno));
	}
}

string codecToString(int codec);

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
			uint16_t codec = read<uint16_t>(file);
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
			case Codec::Pcm:
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
						format("Unsupported sample format: {}-bit PCM.", bitsPerSample));
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
					throw runtime_error(format("Unsupported sample format: {}-bit IEEE Float.", bitsPerSample));
				}
				break;
			default:
				throw runtime_error(format(
					"Unsupported audio codec: '{}'. Only uncompressed codecs ('{}' and '{}') are supported.",
					codecToString(codec), codecToString(Codec::Pcm), codecToString(Codec::Float)));
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

string codecToString(int codec) {
	switch (codec) {
	case 0x0001: return "PCM";
	case 0x0002: return "Microsoft ADPCM";
	case 0x0003: return "IEEE Float";
	case 0x0004: return "Compaq VSELP";
	case 0x0005: return "IBM CVSD";
	case 0x0006: return "Microsoft a-Law";
	case 0x0007: return "Microsoft u-Law";
	case 0x0008: return "Microsoft DTS";
	case 0x0009: return "DRM";
	case 0x000a: return "WMA 9 Speech";
	case 0x000b: return "Microsoft Windows Media RT Voice";
	case 0x0010: return "OKI-ADPCM";
	case 0x0011: return "Intel IMA/DVI-ADPCM";
	case 0x0012: return "Videologic Mediaspace ADPCM";
	case 0x0013: return "Sierra ADPCM";
	case 0x0014: return "Antex G.723 ADPCM";
	case 0x0015: return "DSP Solutions DIGISTD";
	case 0x0016: return "DSP Solutions DIGIFIX";
	case 0x0017: return "Dialoic OKI ADPCM";
	case 0x0018: return "Media Vision ADPCM";
	case 0x0019: return "HP CU";
	case 0x001a: return "HP Dynamic Voice";
	case 0x0020: return "Yamaha ADPCM";
	case 0x0021: return "SONARC Speech Compression";
	case 0x0022: return "DSP Group True Speech";
	case 0x0023: return "Echo Speech Corp.";
	case 0x0024: return "Virtual Music Audiofile AF36";
	case 0x0025: return "Audio Processing Tech.";
	case 0x0026: return "Virtual Music Audiofile AF10";
	case 0x0027: return "Aculab Prosody 1612";
	case 0x0028: return "Merging Tech. LRC";
	case 0x0030: return "Dolby AC2";
	case 0x0031: return "Microsoft GSM610";
	case 0x0032: return "MSN Audio";
	case 0x0033: return "Antex ADPCME";
	case 0x0034: return "Control Resources VQLPC";
	case 0x0035: return "DSP Solutions DIGIREAL";
	case 0x0036: return "DSP Solutions DIGIADPCM";
	case 0x0037: return "Control Resources CR10";
	case 0x0038: return "Natural MicroSystems VBX ADPCM";
	case 0x0039: return "Crystal Semiconductor IMA ADPCM";
	case 0x003a: return "Echo Speech ECHOSC3";
	case 0x003b: return "Rockwell ADPCM";
	case 0x003c: return "Rockwell DIGITALK";
	case 0x003d: return "Xebec Multimedia";
	case 0x0040: return "Antex G.721 ADPCM";
	case 0x0041: return "Antex G.728 CELP";
	case 0x0042: return "Microsoft MSG723";
	case 0x0043: return "IBM AVC ADPCM";
	case 0x0045: return "ITU-T G.726";
	case 0x0050: return "Microsoft MPEG";
	case 0x0051: return "RT23 or PAC";
	case 0x0052: return "InSoft RT24";
	case 0x0053: return "InSoft PAC";
	case 0x0055: return "MP3";
	case 0x0059: return "Cirrus";
	case 0x0060: return "Cirrus Logic";
	case 0x0061: return "ESS Tech. PCM";
	case 0x0062: return "Voxware Inc.";
	case 0x0063: return "Canopus ATRAC";
	case 0x0064: return "APICOM G.726 ADPCM";
	case 0x0065: return "APICOM G.722 ADPCM";
	case 0x0066: return "Microsoft DSAT";
	case 0x0067: return "Micorsoft DSAT DISPLAY";
	case 0x0069: return "Voxware Byte Aligned";
	case 0x0070: return "Voxware AC8";
	case 0x0071: return "Voxware AC10";
	case 0x0072: return "Voxware AC16";
	case 0x0073: return "Voxware AC20";
	case 0x0074: return "Voxware MetaVoice";
	case 0x0075: return "Voxware MetaSound";
	case 0x0076: return "Voxware RT29HW";
	case 0x0077: return "Voxware VR12";
	case 0x0078: return "Voxware VR18";
	case 0x0079: return "Voxware TQ40";
	case 0x007a: return "Voxware SC3";
	case 0x007b: return "Voxware SC3";
	case 0x0080: return "Soundsoft";
	case 0x0081: return "Voxware TQ60";
	case 0x0082: return "Microsoft MSRT24";
	case 0x0083: return "AT&T G.729A";
	case 0x0084: return "Motion Pixels MVI MV12";
	case 0x0085: return "DataFusion G.726";
	case 0x0086: return "DataFusion GSM610";
	case 0x0088: return "Iterated Systems Audio";
	case 0x0089: return "Onlive";
	case 0x008a: return "Multitude, Inc. FT SX20";
	case 0x008b: return "Infocom ITS A/S G.721 ADPCM";
	case 0x008c: return "Convedia G729";
	case 0x008d: return "Not specified congruency, Inc.";
	case 0x0091: return "Siemens SBC24";
	case 0x0092: return "Sonic Foundry Dolby AC3 APDIF";
	case 0x0093: return "MediaSonic G.723";
	case 0x0094: return "Aculab Prosody 8kbps";
	case 0x0097: return "ZyXEL ADPCM";
	case 0x0098: return "Philips LPCBB";
	case 0x0099: return "Studer Professional Audio Packed";
	case 0x00a0: return "Malden PhonyTalk";
	case 0x00a1: return "Racal Recorder GSM";
	case 0x00a2: return "Racal Recorder G720.a";
	case 0x00a3: return "Racal G723.1";
	case 0x00a4: return "Racal Tetra ACELP";
	case 0x00b0: return "NEC AAC NEC Corporation";
	case 0x00ff: return "AAC";
	case 0x0100: return "Rhetorex ADPCM";
	case 0x0101: return "IBM u-Law";
	case 0x0102: return "IBM a-Law";
	case 0x0103: return "IBM ADPCM";
	case 0x0111: return "Vivo G.723";
	case 0x0112: return "Vivo Siren";
	case 0x0120: return "Philips Speech Processing CELP";
	case 0x0121: return "Philips Speech Processing GRUNDIG";
	case 0x0123: return "Digital G.723";
	case 0x0125: return "Sanyo LD ADPCM";
	case 0x0130: return "Sipro Lab ACEPLNET";
	case 0x0131: return "Sipro Lab ACELP4800";
	case 0x0132: return "Sipro Lab ACELP8V3";
	case 0x0133: return "Sipro Lab G.729";
	case 0x0134: return "Sipro Lab G.729A";
	case 0x0135: return "Sipro Lab Kelvin";
	case 0x0136: return "VoiceAge AMR";
	case 0x0140: return "Dictaphone G.726 ADPCM";
	case 0x0150: return "Qualcomm PureVoice";
	case 0x0151: return "Qualcomm HalfRate";
	case 0x0155: return "Ring Zero Systems TUBGSM";
	case 0x0160: return "Microsoft Audio1";
	case 0x0161: return "Windows Media Audio V2 V7 V8 V9 / DivX audio (WMA) / Alex AC3 Audio";
	case 0x0162: return "Windows Media Audio Professional V9";
	case 0x0163: return "Windows Media Audio Lossless V9";
	case 0x0164: return "WMA Pro over S/PDIF";
	case 0x0170: return "UNISYS NAP ADPCM";
	case 0x0171: return "UNISYS NAP ULAW";
	case 0x0172: return "UNISYS NAP ALAW";
	case 0x0173: return "UNISYS NAP 16K";
	case 0x0174: return "MM SYCOM ACM SYC008 SyCom Technologies";
	case 0x0175: return "MM SYCOM ACM SYC701 G726L SyCom Technologies";
	case 0x0176: return "MM SYCOM ACM SYC701 CELP54 SyCom Technologies";
	case 0x0177: return "MM SYCOM ACM SYC701 CELP68 SyCom Technologies";
	case 0x0178: return "Knowledge Adventure ADPCM";
	case 0x0180: return "Fraunhofer IIS MPEG2AAC";
	case 0x0190: return "Digital Theater Systems DTS DS";
	case 0x0200: return "Creative Labs ADPCM";
	case 0x0202: return "Creative Labs FASTSPEECH8";
	case 0x0203: return "Creative Labs FASTSPEECH10";
	case 0x0210: return "UHER ADPCM";
	case 0x0215: return "Ulead DV ACM";
	case 0x0216: return "Ulead DV ACM";
	case 0x0220: return "Quarterdeck Corp.";
	case 0x0230: return "I-Link VC";
	case 0x0240: return "Aureal Semiconductor Raw Sport";
	case 0x0241: return "ESST AC3";
	case 0x0250: return "Interactive Products HSX";
	case 0x0251: return "Interactive Products RPELP";
	case 0x0260: return "Consistent CS2";
	case 0x0270: return "Sony SCX";
	case 0x0271: return "Sony SCY";
	case 0x0272: return "Sony ATRAC3";
	case 0x0273: return "Sony SPC";
	case 0x0280: return "TELUM Telum Inc.";
	case 0x0281: return "TELUMIA Telum Inc.";
	case 0x0285: return "Norcom Voice Systems ADPCM";
	case 0x0300: return "Fujitsu FM TOWNS SND";
	case 0x0301:
	case 0x0302:
	case 0x0303:
	case 0x0304:
	case 0x0305:
	case 0x0306:
	case 0x0307:
	case 0x0308: return "Fujitsu (not specified)";
	case 0x0350: return "Micronas Semiconductors, Inc. Development";
	case 0x0351: return "Micronas Semiconductors, Inc. CELP833";
	case 0x0400: return "Brooktree Digital";
	case 0x0401: return "Intel Music Coder (IMC)";
	case 0x0402: return "Ligos Indeo Audio";
	case 0x0450: return "QDesign Music";
	case 0x0500: return "On2 VP7 On2 Technologies";
	case 0x0501: return "On2 VP6 On2 Technologies";
	case 0x0680: return "AT&T VME VMPCM";
	case 0x0681: return "AT&T TCP";
	case 0x0700: return "YMPEG Alpha (dummy for MPEG-2 compressor)";
	case 0x08ae: return "ClearJump LiteWave (lossless)";
	case 0x1000: return "Olivetti GSM";
	case 0x1001: return "Olivetti ADPCM";
	case 0x1002: return "Olivetti CELP";
	case 0x1003: return "Olivetti SBC";
	case 0x1004: return "Olivetti OPR";
	case 0x1100: return "Lernout & Hauspie";
	case 0x1101: return "Lernout & Hauspie CELP codec";
	case 0x1102:
	case 0x1103:
	case 0x1104: return "Lernout & Hauspie SBC codec";
	case 0x1400: return "Norris Comm. Inc.";
	case 0x1401: return "ISIAudio";
	case 0x1500: return "AT&T Soundspace Music Compression";
	case 0x181c: return "VoxWare RT24 speech codec";
	case 0x181e: return "Lucent elemedia AX24000P Music codec";
	case 0x1971: return "Sonic Foundry LOSSLESS";
	case 0x1979: return "Innings Telecom Inc. ADPCM";
	case 0x1c07: return "Lucent SX8300P speech codec";
	case 0x1c0c: return "Lucent SX5363S G.723 compliant codec";
	case 0x1f03: return "CUseeMe DigiTalk (ex-Rocwell)";
	case 0x1fc4: return "NCT Soft ALF2CD ACM";
	case 0x2000: return "FAST Multimedia DVM";
	case 0x2001: return "Dolby DTS (Digital Theater System)";
	case 0x2002: return "RealAudio 1 / 2 14.4";
	case 0x2003: return "RealAudio 1 / 2 28.8";
	case 0x2004: return "RealAudio G2 / 8 Cook (low bitrate)";
	case 0x2005: return "RealAudio 3 / 4 / 5 Music (DNET)";
	case 0x2006: return "RealAudio 10 AAC (RAAC)";
	case 0x2007: return "RealAudio 10 AAC+ (RACP)";
	case 0x2500: return "Reserved range to 0x2600 Microsoft";
	case 0x3313: return "makeAVIS (ffvfw fake AVI sound from AviSynth scripts)";
	case 0x4143: return "Divio MPEG-4 AAC audio";
	case 0x4201: return "Nokia adaptive multirate";
	case 0x4243: return "Divio G726 Divio, Inc.";
	case 0x434c: return "LEAD Speech";
	case 0x564c: return "LEAD Vorbis";
	case 0x5756: return "WavPack Audio";
	case 0x674f: return "Ogg Vorbis (mode 1)";
	case 0x6750: return "Ogg Vorbis (mode 2)";
	case 0x6751: return "Ogg Vorbis (mode 3)";
	case 0x676f: return "Ogg Vorbis (mode 1+)";
	case 0x6770: return "Ogg Vorbis (mode 2+)";
	case 0x6771: return "Ogg Vorbis (mode 3+)";
	case 0x7000: return "3COM NBX 3Com Corporation";
	case 0x706d: return "FAAD AAC";
	case 0x7a21: return "GSM-AMR (CBR, no SID)";
	case 0x7a22: return "GSM-AMR (VBR, including SID)";
	case 0xa100: return "Comverse Infosys Ltd. G723 1";
	case 0xa101: return "Comverse Infosys Ltd. AVQSBC";
	case 0xa102: return "Comverse Infosys Ltd. OLDSBC";
	case 0xa103: return "Symbol Technologies G729A";
	case 0xa104: return "VoiceAge AMR WB VoiceAge Corporation";
	case 0xa105: return "Ingenient Technologies Inc. G726";
	case 0xa106: return "ISO/MPEG-4 advanced audio Coding";
	case 0xa107: return "Encore Software Ltd G726";
	case 0xa109: return "Speex ACM Codec xiph.org";
	case 0xdfac: return "DebugMode SonicFoundry Vegas FrameServer ACM Codec";
	case 0xf1ac: return "Free Lossless Audio Codec FLAC";
	case 0xfffe: return "Extensible";
	case 0xffff: return "Development";
	}
	return format("{0:#x}", codec);
}