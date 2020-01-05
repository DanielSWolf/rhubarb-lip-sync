package com.rhubarb_lip_sync.audio

import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.max
import kotlin.math.min
import kotlin.math.round
import kotlin.math.roundToInt

typealias CoeffArray = FloatArray

const val SRC_MAX_RATIO = 256

inline class FixedPoint constructor(private val value: Int) {
	companion object {
		private const val SHIFT_BITS = 12
		private const val FP_ONE = (1 shl SHIFT_BITS).toDouble()
		private const val INV_FP_ONE = 1.0 / FP_ONE

		fun fromDouble(doubleValue: Double) = FixedPoint((doubleValue * FP_ONE).roundToInt())
		fun fromInt(intValue: Int) = FixedPoint(intValue shl SHIFT_BITS)
		val ZERO = fromInt(0)
	}

	fun toInt() = value shr SHIFT_BITS
	fun toDouble() = getFractionPart() * INV_FP_ONE

	private fun getFractionPart() = value and ((1 shl SHIFT_BITS) - 1)

	operator fun plus(other: FixedPoint) = FixedPoint(value + other.value)
	operator fun minus(other: FixedPoint) = FixedPoint(value - other.value)
	operator fun div(other: FixedPoint) = value / other.value
	operator fun times(other: Int) = FixedPoint(value * other)

	operator fun compareTo(other: FixedPoint) = value.compareTo(other.value)
}

fun InputStream.readLittleEndianInt32() = read() or (read() shl 8) or (read() shl 16) or (read() shl 24)

class SincFilter(val inputSampleRate: Int, val outputSampleRate: Int) {
	val ratio = outputSampleRate.toDouble() / inputSampleRate

	init {
		if (ratio < (1.0 / SRC_MAX_RATIO) || ratio > (1.0 * SRC_MAX_RATIO)) {
			throw Error("SRC ratio outside [1/$SRC_MAX_RATIO, $SRC_MAX_RATIO] range.")
		}
	}

	var last_position: Double = 0.0

	var in_count: Int = 0
	var in_used: Int = 0
	var out_count: Int = 0
	var out_gen: Int = 0

	companion object {
		// Load coefficients from resource
		val index_inc: Int
		val coeffs: CoeffArray
		val coeff_half_len: Int

		init {
			// Load coefficient data from stream
			val stream = ::SincFilter.javaClass.getResourceAsStream("coeffs.bin")
			check(stream != null) { "Error loading coefficients." }

			index_inc = stream.readLittleEndianInt32()
			val floatBuffer = ByteBuffer.wrap(stream.readBytes())
				.also { it.order(ByteOrder.LITTLE_ENDIAN) }
				.asFloatBuffer()
			stream.close()

			val coeffCount = floatBuffer.limit()
			coeffs = CoeffArray(coeffCount).also { floatBuffer.get(it) }
			coeff_half_len = coeffCount - 2
		}
	}

	var b_current: Int = 0
	var b_end: Int = 0
	var b_real_end: Int = -1
	val b_len: Int = run {
		var length = 3 * ((coeff_half_len + 2.0) / index_inc * SRC_MAX_RATIO + 1).roundToInt()
		length = max(length, 4096)
		// There is a <= check against samples_in_hand requiring a buffer bigger than the calculation above
		length += 1
		return@run length
	}

	val buffer: SampleArray = SampleArray(b_len + 1 /* 1 channel */)

	fun process(data: Data) {
		in_count = data.data_in.size
		out_count = data.data_out.size
		in_used = 0
		out_gen = 0

		// Check the sample rate ratio wrt the buffer len.
		var count: Double = (coeff_half_len + 2.0) / index_inc
		if (ratio < 1.0) count /= ratio

		/* Maximum coefficientson either side of center point. */
		val half_filter_chan_len: Int = count.roundToInt() + 1

		var input_index: Double = last_position

		var rem: Double = fmod_one (input_index)
		b_current = (b_current + (input_index - rem).roundToInt()) % b_len
		input_index = rem

		val terminate: Double = 1.0 / ratio + 1e-20

		/* Main processing loop. */
		while (out_gen < out_count) {
			/* Need to reload buffer? */
			var samples_in_hand: Int = (b_end - b_current + b_len) % b_len

			if (samples_in_hand <= half_filter_chan_len) {
				prepare_data(data, half_filter_chan_len)

				samples_in_hand = (b_end - b_current + b_len) % b_len
				if (samples_in_hand <= half_filter_chan_len) break
			}

			/* This is the termination condition. */
			if (b_real_end >= 0) {
				if (b_current + input_index + terminate > b_real_end) break
			}

			val float_increment: Double = index_inc * (if (ratio < 1.0) ratio else 1.0)
			val increment: FixedPoint = FixedPoint.fromDouble(float_increment)

			val start_filter_index: FixedPoint = FixedPoint.fromDouble(input_index * float_increment)

			data.data_out[out_gen] = ((float_increment / index_inc) * calc_output_single (increment, start_filter_index)).toFloat()
			out_gen++

			/* Figure out the next index. */
			input_index += 1.0 / ratio
			rem = fmod_one(input_index)

			b_current = (b_current + (input_index - rem).roundToInt()) % b_len
			input_index = rem
		}

		last_position = input_index

		data.input_frames_used = in_used
		data.output_frames_gen = out_gen
	}

	private fun prepare_data (data: Data, half_filter_chan_len: Int) {
		if (b_real_end >= 0) return

		var len: Int = 0
		if (b_current == 0) {
			// Initial state.
			// Set up zeros at the start of the buffer and then load new data after that.
			len = b_len - 2 * half_filter_chan_len

			b_current = half_filter_chan_len
			b_end = half_filter_chan_len
		} else if (b_end + half_filter_chan_len + 1 /* 1 channel */ < b_len) {
			/*  Load data at current end position. */
			len = max(b_len - b_current - half_filter_chan_len, 0)
		} else {
			/* Move data at end of buffer back to the start of the buffer. */
			len = b_end - b_current
			System.arraycopy(buffer, b_current - half_filter_chan_len, buffer, 0, half_filter_chan_len + len)

			b_current = half_filter_chan_len
			b_end = b_current + len

			/* Now load data at current end of buffer. */
			len = max(b_len - b_current - half_filter_chan_len, 0)
		}

		len = min(in_count - in_used, len)

		check(len >= 0 && b_end + len <= b_len) { "Internal error: Bad length in prepare_data()." }
		System.arraycopy(data.data_in, in_used, buffer, b_end, len)

		b_end += len
		in_used += len

		if (in_used == in_count && b_end - b_current < 2 * half_filter_chan_len && data.end_of_input) {
			// Handle the case where all data in the current buffer has been consumed and this is
			// the last buffer.

			if (b_len - b_end < half_filter_chan_len + 5) {
				/* If necessary, move data down to the start of the buffer. */
				len = b_end - b_current
				System.arraycopy(buffer, b_current - half_filter_chan_len, buffer, 0, half_filter_chan_len + len)

				b_current = half_filter_chan_len
				b_end = b_current + len
			}

			b_real_end = b_end
			len = half_filter_chan_len + 5

			if (len < 0 || b_end + len > b_len) {
				len = b_len - b_end
			}

			buffer.fill(0.0f, b_end, len)
			b_end += len
		}
	}

	private fun calc_output_single(increment: FixedPoint, start_filter_index: FixedPoint): Double {
		var fraction: Double
		var icoeff: Double
		var data_index: Int
		var coeff_count: Int
		var indx: Int

		/* Convert input parameters into fixed point. */
		val max_filter_index = FixedPoint.fromInt(coeff_half_len)

		/* First apply the left half of the filter. */
		var filter_index: FixedPoint = start_filter_index
		coeff_count = (max_filter_index - filter_index) / increment
		filter_index += increment * coeff_count
		data_index = b_current - coeff_count

		var left = 0.0
		do {
			if (data_index >= 0) {
				/* Avoid underflow access to buffer. */
				fraction = filter_index.toDouble()
				indx = filter_index.toInt()

				icoeff = coeffs [indx] + fraction * (coeffs[indx + 1] - coeffs[indx])

				left += icoeff * buffer[data_index]
			}

			filter_index -= increment
			data_index += 1
		} while (filter_index >= FixedPoint.ZERO)

		/* Now apply the right half of the filter. */
		filter_index = increment - start_filter_index
		coeff_count = (max_filter_index - filter_index) / increment
		filter_index += increment * coeff_count
		data_index = b_current + 1 + coeff_count

		var right = 0.0
		do {
			fraction = filter_index.toDouble()
			indx = filter_index.toInt()

			icoeff = coeffs[indx] + fraction * (coeffs[indx + 1] - coeffs[indx])

			right += icoeff * buffer[data_index]

			filter_index -= increment
			data_index -= 1
		} while (filter_index > FixedPoint.ZERO)

		return left + right
	}
}

class Data(val data_in: SampleArray, val data_out: SampleArray, val end_of_input: Boolean) {
	var input_frames_used: Int = 0
	var output_frames_gen: Int = 0
}

fun fmod_one (x: Double): Double {
	val res = x - round(x)
	return if (res < 0.0) res + 1.0 else res
}
