package voice_activity_detection

import AudioBuffer
import SampleArray
import org.apache.commons.lang3.mutable.MutableInt
import kotlin.math.absoluteValue

/** Minimum energy required to trigger audio signal */
const val MIN_ENERGY = 10

private const val LOG_CONST = 24660 // 160*log10(2) in Q9.
private const val LOG_ENERGY_INT_PART = 14336 // 14 in Q10

private val HP_ZERO_COEFS = intArrayOf(6631, -13262, 6631)
private val HP_POLE_COEFS = intArrayOf(16384, -7756, 5620)

private const val UPPER_ALL_PASS_COEFS_Q15 = 20972 // 0.64
private const val LOWER_ALL_PASS_COEFS_Q15 = 5571 // 0.17

/**
 * Table used by getLeadingZeroCount.
 * For each UInt n that's a sequence of 0 bits followed by a sequence of 1 bits, the entry at index
 * (n * 0x8c0b2891) shr 26 in this table gives the number of zero bits in n.
 */
private val LEADING_ZEROS_TABLE = intArrayOf(
	32, 8,  17, -1, -1, 14, -1, -1, -1, 20, -1, -1, -1, 28, -1, 18,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  26, 25, 24,
	4,  11, 23, 31, 3,  7,  10, 16, 22, 30, -1, -1, 2,  6,  13, 9,
	-1, 15, -1, 21, -1, 29, 19, -1, -1, -1, -1, -1, 1,  27, 5,  12
).apply { assert(size == 64) }

/** Returns the number of leading zero bits in the argument. */
fun getLeadingZeroCount(n: UInt): Int {
	// Normalize n by rounding up to the nearest number that is a sequence of 0 bits followed by a
	// sequence of 1 bits. This number has the same number of leading zeros as the original n.
	// There are exactly 33 such values.
	var normalized = n
	normalized = normalized or (normalized shr 1)
	normalized = normalized or (normalized shr 2)
	normalized = normalized or (normalized shr 4)
	normalized = normalized or (normalized shr 8)
	normalized = normalized or (normalized shr 16)

	// Multiply the modified n with a constant selected (by exhaustive search) such that each of the
	// 33 possible values of n give a product whose 6 most significant bits are unique.
	// Then look up the answer in the table.
	return LEADING_ZEROS_TABLE[((normalized * 0x8c0b2891u) shr 26).toInt()]
}

/**
 * Returns the number of bits by which a signed int can be left-shifted without overflow, or 0 if
 * a == 0.
 */
fun normSigned(a: Int): Int =
	if (a == 0)
		0
	else
		getLeadingZeroCount((if (a < 0) a.inv() else a).toUInt()) - 1

/**
 * Returns the number of bits by which an unsigned int can be left-shifted without overflow, or 0 if
 * a == 0.
 */
fun normUnsigned(a: UInt): Int = if (a == 0u) 0 else getLeadingZeroCount(a)

/** Returns the number of bits needed to represent the specified value. */
fun getBitCount(n: UInt): Int = 32 - getLeadingZeroCount(n)

/**
 * Returns the number of right bit shifts that must be applied to each of the given samples so that,
 * if the squares of the samples are added [times] times, the signed 32-bit addition will not
 * overflow.
 */
fun getScalingSquare(buffer: AudioBuffer, times: Int): Int {
	var maxAbsSample = -1
	for (i in 0 until buffer.size) {
		val absSample = buffer[i].toInt().absoluteValue
		if (absSample > maxAbsSample) {
			maxAbsSample = absSample
		}
	}

	if (maxAbsSample == 0) {
		return 0 // Since norm(0) returns 0
	}

	val t = normSigned(maxAbsSample * maxAbsSample)
	val bitCount = getBitCount(times.toUInt())
	return if (t > bitCount) 0 else bitCount - t
}

data class EnergyResult(
	/**
	 * The number of left bit shifts needed to get the physical energy value, i.e, to get the Q0
	 * value
	 */
	val rightShifts: Int,

	/** The energy value in Q(-[rightShifts]) */
	val energy: Int
)

/** Calculates the energy of an audio buffer. */
fun getEnergy(buffer: AudioBuffer): EnergyResult {
	val scaling = getScalingSquare(buffer, buffer.size)

	var energy = 0
	for (i in 0 until buffer.size) {
		energy += (buffer[i] * buffer[i]) shr scaling
	}

	return EnergyResult(scaling, energy)
}

/**
 * Performs high pass filtering with a cut-off frequency at 80 Hz, if [input] is sampled at 500 Hz.
 * @return Output audio data in the frequency interval 80 to 250 Hz.
 */
fun highPassFilter(input: AudioBuffer, filterState: IntArray): AudioBuffer {
	// The sum of the absolute values of the impulse response:
	// The zero/pole-filter has a max amplification of a single sample of: 1.4546
	// Impulse response: 0.4047 -0.6179 -0.0266  0.1993  0.1035  -0.0194
	// The all-zero section has a max amplification of a single sample of: 1.6189
	// Impulse response: 0.4047 -0.8094  0.4047  0       0        0
	// The all-pole section has a max amplification of a single sample of: 1.9931
	// Impulse response: 1.0000  0.4734 -0.1189 -0.2187 -0.0627   0.04532

	val result = SampleArray(input.size)
	for (i in 0 until input.size) {
		// All-zero section (filter coefficients in Q14).
		var tmp32 = HP_ZERO_COEFS[0] * input[i]
		tmp32 += HP_ZERO_COEFS[1] * filterState[0]
		tmp32 += HP_ZERO_COEFS[2] * filterState[1]
		filterState[1] = filterState[0]
		filterState[0] = input[i].toInt()

		// All-pole section (filter coefficients in Q14).
		tmp32 -= HP_POLE_COEFS[1] * filterState[2]
		tmp32 -= HP_POLE_COEFS[2] * filterState[3]
		filterState[3] = filterState[2]
		filterState[2] = tmp32 shr 14
		result[i] = filterState[2].toShort()
	}

	return AudioBuffer(result)
}

/**
 * Performs all pass filtering, used before splitting the signal into two frequency bands (low pass
 * vs high pass).
 * @param[filterCoefficient] Given in Q15.
 * @param[filterState] State of the filter given in Q(-1).
 * @return Output audio signal given in Q(-1).
 */
fun allPassFilter(input: AudioBuffer, filterCoefficient: Int, filterState: MutableInt): AudioBuffer {
	// The filter can only cause overflow (in the w16 output variable) if more than 4 consecutive
	// input numbers are of maximum value andhas the the same sign as the impulse responses first
	// taps.
	// First 6 taps of the impulse response:
	// 0.6399 0.5905 -0.3779 0.2418 -0.1547 0.0990

	val result = SampleArray((input.size + 1) / 2)
	var state32 = filterState.toInt() * (1 shl 16) // Q15
	for (i in 0 until input.size step 2) {
		val tmp32 = state32 + filterCoefficient * input[i]
		val tmp16 = tmp32 shr 16 // Q(-1)
		result[i / 2] = tmp16.toShort()
		state32 = input[i] * (1 shl 14) - filterCoefficient * tmp16 // Q14
		state32 *= 2 // Q15.
	}
	filterState.setValue(state32 shr 16) // Q(-1)

	return AudioBuffer(result)
}

data class SplitFilterResult(
	val highPassData: AudioBuffer,
	val lowPassData: AudioBuffer
)

/**
 * Splits audio data into an upper (high pass) part and a lower (low pass) part.
 * @param[upperState] State of the upper filter, given in Q(-1).
 * @param[lowerState] State of the lower filter, given in Q(-1).
 */
fun splitFilter(input: AudioBuffer, upperState: MutableInt, lowerState: MutableInt): SplitFilterResult {
	val resultSize = input.size / 2 // Downsampling by 2

	// All-pass filtering upper branch.
	val tempHighPass = allPassFilter(input, UPPER_ALL_PASS_COEFS_Q15, upperState)
	assert(tempHighPass.size == resultSize)

	// All-pass filtering lower branch.
	val tempLowPass = allPassFilter(AudioBuffer(input, 1), LOWER_ALL_PASS_COEFS_Q15, lowerState)
	assert(tempLowPass.size == resultSize)

	// Make LP and HP signals.
	val highPassData = SampleArray(resultSize)
	val lowPassData = SampleArray(resultSize)
	for (i in 0 until resultSize) {
		highPassData[i] = (tempHighPass[i] - tempLowPass[i]).toShort()
		lowPassData[i] = (tempLowPass[i] + tempHighPass[i]).toShort()
	}

	return SplitFilterResult(AudioBuffer(highPassData), AudioBuffer(lowPassData))
}

/**
 * Calculates the energy of the input signal in dB, and also updates an overall [totalEnergy] if
 * necessary.
 * @param[offset] Offset value added to result.
 * @param[totalEnergy] An external energy updated with the energy of the input signal.
 *   NOTE: [totalEnergy] is only updated if [totalEnergy] <= [MIN_ENERGY].
 * @return 10 * log10(energy of input signal) given in Q4.
 */
fun logOfEnergy(input: AudioBuffer, offset: Int, totalEnergy: MutableInt): Int {
	assert(input.size > 0)

	val energyResult = getEnergy(input)
	// totalRightShifts accumulates the number of right shifts performed on energy.
	var totalRightShifts = energyResult.rightShifts
	// energy will be normalized to 15 bits. We use unsigned integer because we eventually will mask
	// out the fractional part.
	var energy = energyResult.energy.toUInt()

	if (energy == 0u) {
		return offset
	}

	// By construction, normalizing to 15 bits is equivalent with 17 leading zeros of an unsigned 32
	// bit value.
	val normalizingRightShifts = 17 - normUnsigned(energy)
	// In a 15 bit representation the leading bit is 2^14. log2(2^14) in Q10 is
	// (14 shl 10), which is what we initialize log2Energy with. For a more detailed derivations,
	// see below.
	var log2Energy = LOG_ENERGY_INT_PART

	totalRightShifts += normalizingRightShifts
	// Normalize energy to 15 bits.
	// totalRightShifts is now the total number of right shifts performed on energy after
	// normalization. This means that energy is in Q(-totalRightShifts).
	energy = if (normalizingRightShifts < 0)
		energy shl -normalizingRightShifts
	else
		energy shr normalizingRightShifts

	// Calculate the energy ofinput in dB, in Q4.
	//
	// 10 * log10("true energy") in Q4 = 2^4 * 10 * log10("true energy") =
	// 160 * log10(energy * 2^totalRightShifts) =
	// 160 * log10(2) * log2(energy * 2^totalRightShifts) =
	// 160 * log10(2) * (log2(energy) + log2(2^totalRightShifts)) =
	// (160 * log10(2)) * (log2(energy) + totalRightShifts) =
	// LOG_CONST * (log2_energy + totalRightShifts)
	//
	// We know by construction that energy is normalized to 15 bits.
	// Hence, energy = 2^14 + frac_Q15, where frac_Q15 is a fractional part in Q15.
	// Further, we'd like log2_energy in Q10
	// log2(energy) in Q10 = 2^10 * log2(2^14 + frac_Q15) =
	// 2^10 * log2(2^14 * (1 + frac_Q15 * 2^-14)) =
	// 2^10 * (14 + log2(1 + frac_Q15 * 2^-14)) ~=
	// (14 shl 10) + 2^10 * (frac_Q15 * 2^-14) =
	// (14 shl 10) + (frac_Q15 * 2^-4) = (14 shl 10) + (frac_Q15 shr 4)
	//
	// Note that frac_Q15 = (energy & 0x00003FFF)

	// Calculate and add the fractional part to log2Energy.
	log2Energy += ((energy and 0x00003FFFu) shr 4).toInt()

	// LOG_CONST is in Q9, log2_energy in Q10 and totalRightShifts in Q0.
	// Note that in our derivation above, we have accounted for an output in Q4.
	var logEnergy = ((LOG_CONST * log2Energy) shr 19) + ((totalRightShifts * LOG_CONST) shr 9)

	if (logEnergy < 0) {
		logEnergy = 0
	}

	logEnergy += offset

	// Update the approximate totalEnergy with the energy of input, if totalEnergy has not exceeded
	// MIN_ENERGY.
	// totalEnergy is used as an energy indicator in getGmmProbability().
	if (totalEnergy.toInt() <= MIN_ENERGY) {
		if (totalRightShifts >= 0) {
			// We know by construction that energy > MIN_ENERGY in Q0, so add an arbitrary value
			// such that totalEnergy exceeds MIN_ENERGY.
			totalEnergy.add(MIN_ENERGY + 1)
		} else {
			// By construction, energy is represented by 15 bits, hence any number of right shifted
			// energy will fit in an Int.
			// In addition, adding the value to totalEnergy is wrap around safe as long as
			// MIN_ENERGY < 8192.
			totalEnergy.add((energy shr -totalRightShifts).toInt()) // Q0.
		}
	}

	return logEnergy
}
