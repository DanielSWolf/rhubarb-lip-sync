package com.rhubarb_lip_sync.audio

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.EOFException
import java.io.FileNotFoundException
import java.io.IOException
import java.io.RandomAccessFile
import java.lang.RuntimeException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.nio.file.Path

/**
 * Creates an [AudioSource] representing the specified WAVE file.
 *
 * @throws FileNotFoundException if the specified file does not exist.
 * @throws InvalidWaveFileException if the specified file is not a valid WAVE file.
 * @throws UnsupportedWaveFileException if the specified file cannot be read due to limitations in the reader.
 */
suspend fun createWaveFileReader(path: Path): AutoCloseableAudioSource = withContext(Dispatchers.IO) {
	val file = RandomAccessFile(path.toString(), "r")
	val waveFileInfo = getWaveFileInfo(LittleEndianReader(file))
	return@withContext when (waveFileInfo.sampleFormat) {
		SampleFormat.UInt8 -> UInt8WaveFileReader(waveFileInfo, file)
		SampleFormat.Int16 -> Int16WaveFileReader(waveFileInfo, file)
		SampleFormat.Int24 -> Int24WaveFileReader(waveFileInfo, file)
		SampleFormat.Int32 -> Int32WaveFileReader(waveFileInfo, file)
		SampleFormat.Float32 -> Float32WaveFileReader(waveFileInfo, file)
		SampleFormat.Float64 -> Float64WaveFileReader(waveFileInfo, file)
	}
}

/** Indicates that a file is not a valid WAVE file. */
class InvalidWaveFileException(details: String? = null)
	: IOException("File is not a valid WAVE file.${details?.let { " $it" } ?: ""}")

/** Indicates that a WAVE file cannot be read due to limitations in the reader. */
class UnsupportedWaveFileException(details: String)
	: IOException("WAVE file cannot be read. $details")

private abstract class AbstractWaveFileReader(
	protected val waveFileInfo: WaveFileInfo,
	private val file: RandomAccessFile
) : AutoCloseableAudioSource {
	override val sampleRate: Int = waveFileInfo.frameRate
	override val size: Int = waveFileInfo.frameCount

	protected fun readByteBuffer(startFrameIndex: Int, endFrameIndex: Int): ByteBuffer {
		if (startFrameIndex < 0 || startFrameIndex > endFrameIndex || endFrameIndex > waveFileInfo.frameCount) {
			throw RuntimeException("Cannot read from frame $startFrameIndex to $endFrameIndex in ${waveFileInfo.frameCount}-frame file.")
		}

		val bufferSize = (endFrameIndex - startFrameIndex).toLong() * waveFileInfo.bytesPerFrame
		if (bufferSize > Int.MAX_VALUE) {
			throw RuntimeException("Cannot acquire $bufferSize-byte buffer.")
		}
		val buffer = ByteBuffer.allocate(bufferSize.toInt())
		buffer.order(ByteOrder.LITTLE_ENDIAN)

		file.channel.read(buffer, waveFileInfo.dataOffset + startFrameIndex * waveFileInfo.bytesPerFrame)
		buffer.position(0)
		return buffer
	}

	override fun close() {
		file.close()
	}
}

private class UInt8WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end)
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0f / 0x80 / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0f
			repeat (channelCount) {
				frameSum += sampleBuffer.getUByte().toInt() - 0x80
			}
			val frame = frameSum * factor
			result[frameIndex] = frame
		}
		return@withContext result
	}
}

private class Int16WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end).asShortBuffer()
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0f / 0x8000 / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0f
			repeat (channelCount) {
				frameSum += sampleBuffer.get()
			}
			val frame = frameSum * factor
			result[frameIndex] = frame
		}
		return@withContext result
	}
}

private class Int24WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end)
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0f / 0x800000 / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0f
			repeat (channelCount) {
				frameSum += sampleBuffer.getInt24()
			}
			val frame = frameSum * factor
			result[frameIndex] = frame
		}
		return@withContext result
	}
}

private class Int32WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end).asIntBuffer()
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0f / 0x80000000 / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0f
			repeat (channelCount) {
				frameSum += sampleBuffer.get()
			}
			val frame = frameSum * factor
			result[frameIndex] = frame
		}
		return@withContext result
	}
}

private class Float32WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end).asFloatBuffer()
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0f / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0f
			repeat (channelCount) {
				frameSum += sampleBuffer.get()
			}
			val frame = frameSum * factor
			result[frameIndex] = frame
		}
		return@withContext result
	}
}

private class Float64WaveFileReader(waveFileInfo: WaveFileInfo, file: RandomAccessFile) :
	AbstractWaveFileReader(waveFileInfo, file)
{
	override suspend fun getSamples(start: Int, end: Int): SampleArray = withContext(Dispatchers.IO) {
		val sampleBuffer = readByteBuffer(start, end).asDoubleBuffer()
		val frameCount = end - start
		val channelCount = waveFileInfo.channelCount

		val result = SampleArray(frameCount)
		val factor = 1.0 / channelCount
		for (frameIndex in 0 until frameCount) {
			var frameSum = 0.0
			repeat (channelCount) {
				frameSum += sampleBuffer.get()
			}
			val frame = frameSum * factor
			result[frameIndex] = frame.toFloat()
		}
		return@withContext result
	}
}

private enum class SampleFormat {
	UInt8,
	Int16,
	Int24,
	Int32,
	Float32,
	Float64
}

private data class WaveFileInfo(
	val sampleFormat: SampleFormat,
	val channelCount: Int,
	// The number of frames per second, one frame consisting of one sample per channel
	val frameRate: Int,
	val frameCount: Int,
	val bytesPerFrame: Int,
	// The offset, in bytes, of the raw audio data within the file
	val dataOffset: Long
)

private fun getWaveFileInfo(reader: LittleEndianReader): WaveFileInfo {
	try {
		val rootChunkId = reader.readFourCC()
		if (rootChunkId != "RIFF") {
			throw InvalidWaveFileException()
		}

		val rootChunkSize: Long = reader.readDWord()
		val expectedFileSize: Long = reader.position + rootChunkSize
		if (reader.size < expectedFileSize) {
			throw InvalidWaveFileException("File is incomplete.")
		}

		val waveId = reader.readFourCC()
		if (waveId != "WAVE") {
			throw InvalidWaveFileException("File format is not WAVE, but $waveId.")
		}

		data class FormatInfo(val sampleFormat: SampleFormat, val channelCount: Int, val frameRate: Int, val bytesPerFrame: Int)
		data class DataInfo(val dataOffset: Long, val dataByteCount: Long)

		var formatInfo: FormatInfo? = null
		var dataInfo: DataInfo? = null

		while (reader.position < expectedFileSize && (formatInfo == null || dataInfo == null)) {
			val chunkId = reader.readFourCC()
			val chunkSize: Long = reader.readDWord()
			val chunkEnd: Long = roundUpToEven(reader.position + chunkSize)
			when (chunkId) {
				"fmt " -> {
					// Format chunk
					var codec = Codec(reader.readWord())
					val channelCount = reader.readWord().also {
						// This is no technical limitation, just common sense.
						val maxChannelCount = 6
						if (it > maxChannelCount) {
							throw UnsupportedWaveFileException("Channel count $it exceeds maximum value of $maxChannelCount.")
						}
					}
					val frameRate = reader.readDWord().let { longFrameRate ->
						// Technically, it would be enough to check that the frame rate doesn't
						// exceed `Int.MAX_VALUE`.
						val maxSupportedFrameRate = 192_000
						if (longFrameRate > maxSupportedFrameRate) {
							// Although it's technically the *frame* rate, use the more common term
							// *sample* rate for error message.
							throw UnsupportedWaveFileException("Sample rate $longFrameRate exceeds maximum of $maxSupportedFrameRate.")
						}
						return@let longFrameRate.toInt()
					}
					reader.skipBytes(4) // Skip bytes per second
					val bytesPerFrame = reader.readWord()
					val bitsPerSampleOnDisk = reader.readWord()
					var bitsPerSample = bitsPerSampleOnDisk
					if (chunkSize > 16) {
						val extensionSize = reader.readWord()
						if (extensionSize >= 22) {
							// Read extension fields
							bitsPerSample = reader.readWord()
							reader.skipBytes(4) // Skip channel mask
							val codecOverride = Codec(reader.readWord())
							if (codec == Codec.Extensible) {
								codec = codecOverride
							}
						}
					}
					val (sampleFormat, bytesPerSample) = when (codec) {
						Codec.Pcm -> when (bitsPerSample) {
							8 -> Pair(SampleFormat.UInt8, 1)
							16 -> Pair(SampleFormat.Int16, 2)
							24 -> Pair(SampleFormat.Int24, 3)
							32 -> Pair(SampleFormat.Int32, 4)
							else -> throw UnsupportedWaveFileException("Unsupported bit depth for $codec data: $bitsPerSample bits per sample.")
						}
						Codec.Float -> when (bitsPerSample) {
							32 -> Pair(SampleFormat.Float32, 4)
							64 -> Pair(SampleFormat.Float64, 8)
							else -> throw UnsupportedWaveFileException("Unsupported bit depth for $codec data: $bitsPerSample bits per sample.")
						}
						else -> throw UnsupportedWaveFileException(
							"Unsupported audio codec: $codec. Only the uncompressed codecs ${Codec.Pcm} and ${Codec.Float} are supported.")
					}
					if (bytesPerFrame != bytesPerSample * channelCount) {
						throw UnsupportedWaveFileException("Unsupported sample structure.")
					}
					formatInfo = FormatInfo(sampleFormat, channelCount, frameRate, bytesPerFrame)
				}
				"data" -> {
					// Data chunk
					val dataOffset: Long = reader.position
					val dataByteCount = chunkSize
					dataInfo = DataInfo(dataOffset, dataByteCount)
				}
			}
			reader.position = chunkEnd
		}

		if (formatInfo == null) throw InvalidWaveFileException("Missing format chunk.")
		if (dataInfo == null) throw InvalidWaveFileException("Missing data chunk.")

		val frameCount: Long = dataInfo.dataByteCount / formatInfo.bytesPerFrame
		if (frameCount > Int.MAX_VALUE) {
			throw UnsupportedWaveFileException("Cannot read audio file with more than ${Int.MAX_VALUE} samples per channel.")
		}
		return WaveFileInfo(
			formatInfo.sampleFormat,
			formatInfo.channelCount,
			formatInfo.frameRate,
			frameCount.toInt(),
			formatInfo.bytesPerFrame,
			dataInfo.dataOffset
		)
	} catch (e: EOFException) {
		throw InvalidWaveFileException("Unexpected end of file.")
	}
}

private fun roundUpToEven(i: Long) = (i + 1) and 1.inv()

class LittleEndianReader(private val file: RandomAccessFile) {
	private fun readByte(): Int = file.readUnsignedByte()

	/** Reads an unsigned 16-bit word and returns it as an Int. */
	fun readWord(): Int = readByte() or (readByte() shl 8)

	/** Reads an unsigned 32-bit double-word and returns it as a Long. */
	fun readDWord(): Long =
		(readByte() or (readByte() shl 8) or (readByte() shl 16) or (readByte() shl 24)).toUInt().toLong()

	fun readFourCC(): String {
		val bytes = ByteArray(4)
		file.readFully(bytes)
		return String(bytes, StandardCharsets.US_ASCII)
	}

	val size: Long = file.length()

	var position: Long
		get() = file.filePointer
		set(value) = file.seek(value)

	fun skipBytes(byteCount: Int) = file.skipBytes(byteCount)
}

private fun ByteBuffer.getUByte() = this.get().toUByte()

private fun ByteBuffer.getInt24() =
	((getUByte().toInt() shl 8) or (getUByte().toInt() shl 16) or (getUByte().toInt() shl 24)) shr 8

private inline class Codec(val value: Int) {
	companion object {
		val Pcm = Codec(0x0001)
		val Float = Codec(0x0003)
		val Extensible = Codec(0xfffe)
	}

	override fun toString() = when (value) {
		0x0001 -> "PCM"
		0x0002 -> "Microsoft ADPCM"
		0x0003 -> "IEEE Float"
		0x0004 -> "Compaq VSELP"
		0x0005 -> "IBM CVSD"
		0x0006 -> "Microsoft A-law"
		0x0007 -> "Microsoft µ-law"
		0x0008 -> "Microsoft DTS"
		0x0009 -> "DRM"
		0x000a -> "WMA 9 Speech"
		0x000b -> "Microsoft Windows Media RT Voice"
		0x0010 -> "OKI-ADPCM"
		0x0011 -> "Intel IMA/DVI-ADPCM"
		0x0012 -> "Videologic Mediaspace ADPCM"
		0x0013 -> "Sierra ADPCM"
		0x0014 -> "Antex G.723 ADPCM"
		0x0015 -> "DSP Solutions DIGISTD"
		0x0016 -> "DSP Solutions DIGIFIX"
		0x0017 -> "Dialoic OKI ADPCM"
		0x0018 -> "Media Vision ADPCM"
		0x0019 -> "HP CU"
		0x001a -> "HP Dynamic Voice"
		0x0020 -> "Yamaha ADPCM"
		0x0021 -> "SONARC Speech Compression"
		0x0022 -> "DSP Group True Speech"
		0x0023 -> "Echo Speech Corp."
		0x0024 -> "Virtual Music Audiofile AF36"
		0x0025 -> "Audio Processing Tech."
		0x0026 -> "Virtual Music Audiofile AF10"
		0x0027 -> "Aculab Prosody 1612"
		0x0028 -> "Merging Tech. LRC"
		0x0030 -> "Dolby AC2"
		0x0031 -> "Microsoft GSM610"
		0x0032 -> "MSN Audio"
		0x0033 -> "Antex ADPCME"
		0x0034 -> "Control Resources VQLPC"
		0x0035 -> "DSP Solutions DIGIREAL"
		0x0036 -> "DSP Solutions DIGIADPCM"
		0x0037 -> "Control Resources CR10"
		0x0038 -> "Natural MicroSystems VBX ADPCM"
		0x0039 -> "Crystal Semiconductor IMA ADPCM"
		0x003a -> "Echo Speech ECHOSC3"
		0x003b -> "Rockwell ADPCM"
		0x003c -> "Rockwell DIGITALK"
		0x003d -> "Xebec Multimedia"
		0x0040 -> "Antex G.721 ADPCM"
		0x0041 -> "Antex G.728 CELP"
		0x0042 -> "Microsoft MSG723"
		0x0043 -> "IBM AVC ADPCM"
		0x0045 -> "ITU-T G.726"
		0x0050 -> "Microsoft MPEG"
		0x0051 -> "RT23 or PAC"
		0x0052 -> "InSoft RT24"
		0x0053 -> "InSoft PAC"
		0x0055 -> "MP3"
		0x0059 -> "Cirrus"
		0x0060 -> "Cirrus Logic"
		0x0061 -> "ESS Tech. PCM"
		0x0062 -> "Voxware Inc."
		0x0063 -> "Canopus ATRAC"
		0x0064 -> "APICOM G.726 ADPCM"
		0x0065 -> "APICOM G.722 ADPCM"
		0x0066 -> "Microsoft DSAT"
		0x0067 -> "Micorsoft DSAT DISPLAY"
		0x0069 -> "Voxware Byte Aligned"
		0x0070 -> "Voxware AC8"
		0x0071 -> "Voxware AC10"
		0x0072 -> "Voxware AC16"
		0x0073 -> "Voxware AC20"
		0x0074 -> "Voxware MetaVoice"
		0x0075 -> "Voxware MetaSound"
		0x0076 -> "Voxware RT29HW"
		0x0077 -> "Voxware VR12"
		0x0078 -> "Voxware VR18"
		0x0079 -> "Voxware TQ40"
		0x007a -> "Voxware SC3"
		0x007b -> "Voxware SC3"
		0x0080 -> "Soundsoft"
		0x0081 -> "Voxware TQ60"
		0x0082 -> "Microsoft MSRT24"
		0x0083 -> "AT&T G.729A"
		0x0084 -> "Motion Pixels MVI MV12"
		0x0085 -> "DataFusion G.726"
		0x0086 -> "DataFusion GSM610"
		0x0088 -> "Iterated Systems Audio"
		0x0089 -> "Onlive"
		0x008a -> "Multitude, Inc. FT SX20"
		0x008b -> "Infocom ITS A/S G.721 ADPCM"
		0x008c -> "Convedia G729"
		0x008d -> "Not specified congruency, Inc."
		0x0091 -> "Siemens SBC24"
		0x0092 -> "Sonic Foundry Dolby AC3 APDIF"
		0x0093 -> "MediaSonic G.723"
		0x0094 -> "Aculab Prosody 8kbps"
		0x0097 -> "ZyXEL ADPCM"
		0x0098 -> "Philips LPCBB"
		0x0099 -> "Studer Professional Audio Packed"
		0x00a0 -> "Malden PhonyTalk"
		0x00a1 -> "Racal Recorder GSM"
		0x00a2 -> "Racal Recorder G720.a"
		0x00a3 -> "Racal G723.1"
		0x00a4 -> "Racal Tetra ACELP"
		0x00b0 -> "NEC AAC NEC Corporation"
		0x00ff -> "AAC"
		0x0100 -> "Rhetorex ADPCM"
		0x0101 -> "IBM u-Law"
		0x0102 -> "IBM a-Law"
		0x0103 -> "IBM ADPCM"
		0x0111 -> "Vivo G.723"
		0x0112 -> "Vivo Siren"
		0x0120 -> "Philips Speech Processing CELP"
		0x0121 -> "Philips Speech Processing GRUNDIG"
		0x0123 -> "Digital G.723"
		0x0125 -> "Sanyo LD ADPCM"
		0x0130 -> "Sipro Lab ACEPLNET"
		0x0131 -> "Sipro Lab ACELP4800"
		0x0132 -> "Sipro Lab ACELP8V3"
		0x0133 -> "Sipro Lab G.729"
		0x0134 -> "Sipro Lab G.729A"
		0x0135 -> "Sipro Lab Kelvin"
		0x0136 -> "VoiceAge AMR"
		0x0140 -> "Dictaphone G.726 ADPCM"
		0x0150 -> "Qualcomm PureVoice"
		0x0151 -> "Qualcomm HalfRate"
		0x0155 -> "Ring Zero Systems TUBGSM"
		0x0160 -> "Microsoft Audio1"
		0x0161 -> "Windows Media Audio V2 V7 V8 V9 / DivX audio (WMA) / Alex AC3 Audio"
		0x0162 -> "Windows Media Audio Professional V9"
		0x0163 -> "Windows Media Audio Lossless V9"
		0x0164 -> "WMA Pro over S/PDIF"
		0x0170 -> "UNISYS NAP ADPCM"
		0x0171 -> "UNISYS NAP ULAW"
		0x0172 -> "UNISYS NAP ALAW"
		0x0173 -> "UNISYS NAP 16K"
		0x0174 -> "MM SYCOM ACM SYC008 SyCom Technologies"
		0x0175 -> "MM SYCOM ACM SYC701 G726L SyCom Technologies"
		0x0176 -> "MM SYCOM ACM SYC701 CELP54 SyCom Technologies"
		0x0177 -> "MM SYCOM ACM SYC701 CELP68 SyCom Technologies"
		0x0178 -> "Knowledge Adventure ADPCM"
		0x0180 -> "Fraunhofer IIS MPEG2AAC"
		0x0190 -> "Digital Theater Systems DTS DS"
		0x0200 -> "Creative Labs ADPCM"
		0x0202 -> "Creative Labs FASTSPEECH8"
		0x0203 -> "Creative Labs FASTSPEECH10"
		0x0210 -> "UHER ADPCM"
		0x0215 -> "Ulead DV ACM"
		0x0216 -> "Ulead DV ACM"
		0x0220 -> "Quarterdeck Corp."
		0x0230 -> "I-Link VC"
		0x0240 -> "Aureal Semiconductor Raw Sport"
		0x0241 -> "ESST AC3"
		0x0250 -> "Interactive Products HSX"
		0x0251 -> "Interactive Products RPELP"
		0x0260 -> "Consistent CS2"
		0x0270 -> "Sony SCX"
		0x0271 -> "Sony SCY"
		0x0272 -> "Sony ATRAC3"
		0x0273 -> "Sony SPC"
		0x0280 -> "TELUM Telum Inc."
		0x0281 -> "TELUMIA Telum Inc."
		0x0285 -> "Norcom Voice Systems ADPCM"
		0x0300 -> "Fujitsu FM TOWNS SND"
		0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308 -> "Fujitsu (not specified)"
		0x0350 -> "Micronas Semiconductors, Inc. Development"
		0x0351 -> "Micronas Semiconductors, Inc. CELP833"
		0x0400 -> "Brooktree Digital"
		0x0401 -> "Intel Music Coder (IMC)"
		0x0402 -> "Ligos Indeo Audio"
		0x0450 -> "QDesign Music"
		0x0500 -> "On2 VP7 On2 Technologies"
		0x0501 -> "On2 VP6 On2 Technologies"
		0x0680 -> "AT&T VME VMPCM"
		0x0681 -> "AT&T TCP"
		0x0700 -> "YMPEG Alpha (dummy for MPEG-2 compressor)"
		0x08ae -> "ClearJump LiteWave (lossless)"
		0x1000 -> "Olivetti GSM"
		0x1001 -> "Olivetti ADPCM"
		0x1002 -> "Olivetti CELP"
		0x1003 -> "Olivetti SBC"
		0x1004 -> "Olivetti OPR"
		0x1100 -> "Lernout & Hauspie"
		0x1101 -> "Lernout & Hauspie CELP codec"
		0x1102, 0x1103, 0x1104 -> "Lernout & Hauspie SBC codec"
		0x1400 -> "Norris Comm. Inc."
		0x1401 -> "ISIAudio"
		0x1500 -> "AT&T Soundspace Music Compression"
		0x181c -> "VoxWare RT24 speech codec"
		0x181e -> "Lucent elemedia AX24000P Music codec"
		0x1971 -> "Sonic Foundry LOSSLESS"
		0x1979 -> "Innings Telecom Inc. ADPCM"
		0x1c07 -> "Lucent SX8300P speech codec"
		0x1c0c -> "Lucent SX5363S G.723 compliant codec"
		0x1f03 -> "CUseeMe DigiTalk (ex-Rocwell)"
		0x1fc4 -> "NCT Soft ALF2CD ACM"
		0x2000 -> "FAST Multimedia DVM"
		0x2001 -> "Dolby DTS (Digital Theater System)"
		0x2002 -> "RealAudio 1 / 2 14.4"
		0x2003 -> "RealAudio 1 / 2 28.8"
		0x2004 -> "RealAudio G2 / 8 Cook (low bitrate)"
		0x2005 -> "RealAudio 3 / 4 / 5 Music (DNET)"
		0x2006 -> "RealAudio 10 AAC (RAAC)"
		0x2007 -> "RealAudio 10 AAC+ (RACP)"
		0x2500 -> "Reserved range to 0x2600 Microsoft"
		0x3313 -> "makeAVIS (ffvfw fake AVI sound from AviSynth scripts)"
		0x4143 -> "Divio MPEG-4 AAC audio"
		0x4201 -> "Nokia adaptive multirate"
		0x4243 -> "Divio G726 Divio, Inc."
		0x434c -> "LEAD Speech"
		0x564c -> "LEAD Vorbis"
		0x5756 -> "WavPack Audio"
		0x674f -> "Ogg Vorbis (mode 1)"
		0x6750 -> "Ogg Vorbis (mode 2)"
		0x6751 -> "Ogg Vorbis (mode 3)"
		0x676f -> "Ogg Vorbis (mode 1+)"
		0x6770 -> "Ogg Vorbis (mode 2+)"
		0x6771 -> "Ogg Vorbis (mode 3+)"
		0x7000 -> "3COM NBX 3Com Corporation"
		0x706d -> "FAAD AAC"
		0x7a21 -> "GSM-AMR (CBR, no SID)"
		0x7a22 -> "GSM-AMR (VBR, including SID)"
		0xa100 -> "Comverse Infosys Ltd. G723 1"
		0xa101 -> "Comverse Infosys Ltd. AVQSBC"
		0xa102 -> "Comverse Infosys Ltd. OLDSBC"
		0xa103 -> "Symbol Technologies G729A"
		0xa104 -> "VoiceAge AMR WB VoiceAge Corporation"
		0xa105 -> "Ingenient Technologies Inc. G726"
		0xa106 -> "ISO/MPEG-4 advanced audio Coding"
		0xa107 -> "Encore Software Ltd G726"
		0xa109 -> "Speex ACM Codec xiph.org"
		0xdfac -> "DebugMode SonicFoundry Vegas FrameServer ACM Codec"
		0xf1ac -> "Free Lossless Audio Codec FLAC"
		0xfffe -> "Extensible"
		0xffff -> "Development"
		else -> "0x%04X".format(value)
	}
}
