package com.rhubarb_lip_sync.audio

import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.assertj.core.api.Assertions.assertThat
import org.assertj.core.api.Assertions.assertThatCode
import org.assertj.core.api.Assertions.assertThatThrownBy
import org.assertj.core.api.Assertions.within
import org.spekframework.spek2.Spek
import org.spekframework.spek2.style.specification.describe
import java.io.FileNotFoundException
import java.io.IOException
import java.io.RandomAccessFile
import java.lang.Integer.max
import java.lang.RuntimeException
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.nio.file.StandardCopyOption
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.sin

object WaveFileReaderTest: Spek({
	describe("createWaveFileReader") {
		it("throws FileNotFoundException if the file does not exist") {
			assertThatThrownBy { runBlocking { createWaveFileReader(Paths.get("this file does not exist")) } }
				.isInstanceOf(FileNotFoundException::class.java)
		}

		it("throws InvalidWaveFileException if the file has zero bytes") {
			assertThatThrownBy { runBlocking { createWaveFileReader(getAudioFilePath("zero-bytes.wav")) } }
				.isInstanceOf(InvalidWaveFileException::class.java)
				.hasMessage("File is not a valid WAVE file. Unexpected end of file.")
		}

		it("throws InvalidWaveFileException if the file is incomplete") {
			assertThatThrownBy { runBlocking { createWaveFileReader(getAudioFilePath("incomplete.wav")) } }
				.isInstanceOf(InvalidWaveFileException::class.java)
				.hasMessage("File is not a valid WAVE file. File is incomplete.")
		}

		it("throws InvalidWaveFileException if the file format does not match") {
			assertThatThrownBy { runBlocking { createWaveFileReader(getAudioFilePath("sine-triangle.ogg")) } }
				.isInstanceOf(InvalidWaveFileException::class.java)
				.hasMessage("File is not a valid WAVE file.")
		}

		it("throws UnsupportedWaveFileException if the encoding is not supported") {
			assertThatThrownBy { runBlocking { createWaveFileReader(getAudioFilePath("sine-triangle-flac-ffmpeg.wav")) } }
				.isInstanceOf(UnsupportedWaveFileException::class.java)
				.hasMessage("WAVE file cannot be read. Unsupported audio codec: Free Lossless Audio Codec FLAC. Only the uncompressed codecs PCM and IEEE Float are supported.")
		}

		it("throws UnsupportedWaveFileException if the codec is unknown") {
			assertThatThrownBy { runBlocking { createWaveFileReader(getAudioFilePath("sine-triangle-vorbis-ffmpeg.wav")) } }
				.isInstanceOf(UnsupportedWaveFileException::class.java)
				.hasMessage("WAVE file cannot be read. Unsupported audio codec: 0x566F. Only the uncompressed codecs PCM and IEEE Float are supported.")
		}

		it("does not throw on valid file") {
			assertThatCode {
				runBlocking { createWaveFileReader(getAudioFilePath("sine-triangle-float32-ffmpeg.wav")) }
			}.doesNotThrowAnyException()
		}

		it("throws on invalid zero-length file") {
			// Zero-length files created by Adobe Audition are missing their data chunk
			assertThatThrownBy {
				runBlocking { createWaveFileReader(getAudioFilePath("zero-frames-audition.wav")) }
			}.hasMessage("File is not a valid WAVE file. Missing data chunk.")
		}

		it("does not throw on valid zero-length file") {
			assertThatCode {
				runBlocking { createWaveFileReader(getAudioFilePath("zero-frames-ffmpeg.wav")) }
			}.doesNotThrowAnyException()
		}

		listOf(
			"sine-triangle-uint8-audition.wav",
			"sine-triangle-uint8-ffmpeg.wav",
			"sine-triangle-uint8-soundforge.wav",
			"sine-triangle-int16-audacity.wav",
			"sine-triangle-int16-audition.wav",
			"sine-triangle-int16-ffmpeg.wav",
			"sine-triangle-int16-soundforge.wav",
			"sine-triangle-int24-audacity.wav",
			"sine-triangle-int24-audition.wav",
			"sine-triangle-int24-ffmpeg.wav",
			"sine-triangle-int24-soundforge.wav",
			"sine-triangle-int32-ffmpeg.wav",
			"sine-triangle-int32-soundforge.wav",
			"sine-triangle-float32-audacity.wav",
			"sine-triangle-float32-audition.wav",
			"sine-triangle-float32-ffmpeg.wav",
			"sine-triangle-float32-soundforge.wav",
			"sine-triangle-float64-ffmpeg.wav"
		).forEach { fileName ->
			val path = getAudioFilePath(fileName)

			describe("reader for $fileName") {
				describe("sampleRate") {
					it("is 48000 Hz") {
						val reader = runBlocking { createWaveFileReader(path) }
						assertThat(reader.sampleRate).isEqualTo(48000)
					}
				}

				val expectedSampleCount = 10 * 48000
				describe("size") {
					it("is $expectedSampleCount samples") {
						val reader = runBlocking { createWaveFileReader(path) }
						assertThat(reader.size).isEqualTo(expectedSampleCount)
					}
				}

				val epsilon = when {
					"uint8" in fileName -> 1.0f / 0x80
					"int16" in fileName -> 1.0f / 0x8000
					"int24" in fileName -> 1.0f / 0x800000
					"int32" in fileName -> 1.0f / 0x80000000
					"float" in fileName -> 0.0f
					else -> throw RuntimeException("File name $fileName does not contain sample format information.")
				}

				describe("getSamples") {
					it("throws if arguments are out of bounds") {
						val reader = runBlocking { createWaveFileReader(path) }
						assertThatThrownBy { runBlocking { reader.getSamples(-1, 1) } }
							.hasMessage("Cannot read from frame -1 to 1 in 480000-frame file.")
						assertThatThrownBy { runBlocking { reader.getSamples(10, 9) } }
							.hasMessage("Cannot read from frame 10 to 9 in 480000-frame file.")
						assertThatThrownBy { runBlocking { reader.getSamples(reader.size - 1, reader.size + 1) } }
							.hasMessage("Cannot read from frame 479999 to 480001 in 480000-frame file.")
					}

					it("correctly reads the entire file") {
						runBlocking {
							val reader = createWaveFileReader(path)
							val samples = reader.getSamples(0, reader.size)

							for (sampleIndex in samples.indices) {
								assertThat(samples[sampleIndex])
									.describedAs("sample $sampleIndex")
									.isCloseTo(getSineTriangleSample(sampleIndex), within(epsilon))
							}
						}
					}

					it("correctly reads overlapping blocks in parallel") {
						runBlocking {
							val reader = createWaveFileReader(path)
							val blockCount = 5
							val overlap = 10

							coroutineScope {
								for (i in 0 until blockCount) {
									launch {
										val start = max(0, reader.size * i / blockCount - overlap)
										val end = reader.size * (i + 1) / blockCount
										val samples = reader.getSamples(start, end)
										for (sampleIndex in start until end ) {
											assertThat(samples[sampleIndex -start])
												.describedAs("sample $sampleIndex")
												.isCloseTo(getSineTriangleSample(sampleIndex), within(epsilon))
										}
									}
								}
							}
						}
					}

					it("allows reading zero samples") {
						val reader = runBlocking { createWaveFileReader(path) }
						for (sampleIndex in listOf(0, 10, expectedSampleCount - 1, expectedSampleCount)) {
							assertThat(runBlocking { reader.getSamples(sampleIndex, sampleIndex) })
								.isEqualTo(SampleArray(0))
						}
					}
				}

				describe("close") {
					it("frees all file handles") {
						// Open a temporary copy
						val tempPath = createTempFile().toPath()
						Files.copy(path, tempPath, StandardCopyOption.REPLACE_EXISTING)
						runBlocking { createWaveFileReader(tempPath) }.use {
							// File cannot be deleted while open
							assertThatThrownBy { Files.delete(tempPath) }.isInstanceOf(IOException::class.java)
						}
						// File can be deleted once closed
						assertThatCode { Files.delete(tempPath) }.doesNotThrowAnyException()
					}

					it("does not throw if invoked repeatedly") {
						val reader = runBlocking { createWaveFileReader(path) }
						repeat(3) {
							assertThatCode { reader.close() }.doesNotThrowAnyException()
						}
					}
				}
			}
		}

	}
})

fun getAudioFilePath(pathSuffix: String): Path {
	val resource = WaveFileReaderTest::class.java.classLoader.getResource("audio/WaveFileReader/$pathSuffix")
		?: throw Exception("Could not locate resource $pathSuffix.")
	return Paths.get(resource.toURI())
}

// Returns one downmixed sample of the test signal.
fun getSineTriangleSample(sampleIndex: Int): Float {
	val (left, right) = getSineTriangleFrame(sampleIndex)
	return (left + right) / 2.0f
}

// Returns the left and right sample of one frame of the test signal.
// The test signal consists of a 1 kHz sine wave (at 48 kHz sample rate) on the left channel and a
// 1 kHz triangle wave on the right channel.
fun getSineTriangleFrame(sampleIndex: Int): Pair<Float, Float> {
	val period = 48 // Samples per period
	val t: Float = sampleIndex.rem(period).toFloat() / period // [0, 1[

	// The test signal contains a sine wave on one channel, a triangle wave on the other.
	// Return the downmixed mono result.
	val sine = sin(t * 2 * PI.toFloat())
	val triangle = abs((t + 0.75f).rem(1f) * 4f - 2f) - 1f
	return Pair(sine, triangle)
}

// Generates the raw test signal file from which all other files are derived.
// Output format is big-endian 32-bit IEEE floats, stereo, 48,000 Hz.
// FFmpeg command line: `ffmpeg -f f32be -ar 48000 -ac 2 -i sine-triangle.raw -acodec pcm_f32le sine-triangle-float32-ffmpeg.wav`
@Suppress("unused")
fun generateRawFile() {
	RandomAccessFile("${System.getProperty("user.home")}/sine-triangle.raw", "rw").use { file ->
		for (sampleIndex in 0 until 48 * 1000 * 10) {
			val (left, right) = getSineTriangleFrame(sampleIndex)
			file.writeFloat(left)
			file.writeFloat(right)
		}
	}
}
