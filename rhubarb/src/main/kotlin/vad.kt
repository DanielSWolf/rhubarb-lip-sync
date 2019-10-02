import org.apache.commons.lang3.mutable.MutableInt
import kotlin.math.absoluteValue

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/signal_processing/include/signal_processing_library.h

// Macros specific for the fixed point implementation
val WEBRTC_SPL_WORD16_MAX = 32767

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/signal_processing/include/spl_inl.h
// webrtc/common_audio/signal_processing/spl_inl.c

// Table used by CountLeadingZeros32_NotBuiltin. For each UInt n
// that's a sequence of 0 bits followed by a sequence of 1 bits, the entry at
// index (n * 0x8c0b2891) shr 26 in this table gives the number of zero bits in
// n.
val kCountLeadingZeros32_Table = intArrayOf(
	32, 8,  17, -1, -1, 14, -1, -1, -1, 20, -1, -1, -1, 28, -1, 18,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  26, 25, 24,
	4,  11, 23, 31, 3,  7,  10, 16, 22, 30, -1, -1, 2,  6,  13, 9,
	-1, 15, -1, 21, -1, 29, 19, -1, -1, -1, -1, -1, 1,  27, 5,  12
).apply { assert(size == 64) }

// Returns the number of leading zero bits in the argument.
fun CountLeadingZeros32(n: UInt): Int {
	// Normalize n by rounding up to the nearest number that is a sequence of 0
	// bits followed by a sequence of 1 bits. This number has the same number of
	// leading zeros as the original n. There are exactly 33 such values.
	var normalized = n or (n shr 1)
	normalized = normalized or (normalized shr 2)
	normalized = normalized or (normalized shr 4)
	normalized = normalized or (normalized shr 8)
	normalized = normalized or (normalized shr 16)

	// Multiply the modified n with a constant selected (by exhaustive search)
	// such that each of the 33 possible values of n give a product whose 6 most
	// significant bits are unique. Then look up the answer in the table.
	return kCountLeadingZeros32_Table[((normalized * 0x8c0b2891u) shr 26).toInt()]
}

// Return the number of steps a signed int can be left-shifted without overflow,
// or 0 if a == 0.
inline fun NormW32(a: Int): Int {
	return if (a == 0)
		0
	else
		CountLeadingZeros32((if (a < 0) a.inv() else a).toUInt()) - 1
}

// Return the number of steps an unsigned int can be left-shifted without overflow,
// or 0 if a == 0.
inline fun NormU32(a: UInt): Int {
	return if (a == 0u) 0 else CountLeadingZeros32(a)
}

inline fun GetSizeInBits(n: UInt): Int {
	return 32 - CountLeadingZeros32(n)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/signal_processing/get_scaling_square.c

//
// GetScalingSquare(...)
//
// Returns the # of bits required to scale the samples specified in the
// [in_vector] parameter so that, if the squares of the samples are added the
// # of times specified by the [times] parameter, the 32-bit addition will not
// overflow (result in Int).
//
// Input:
//      - in_vector         : Input vector to check scaling on
//      - in_vector_length  : Samples in [in_vector]
//      - times             : Number of additions to be performed
//
// Return value             : Number of right bit shifts needed to avoid
//                            overflow in the addition calculation
fun GetScalingSquare(buffer: AudioBuffer, times: Int): Int {
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

	val t = NormW32(maxAbsSample * maxAbsSample)
	val bitCount = GetSizeInBits(times.toUInt())
	return if (t > bitCount) 0 else bitCount - t
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/signal_processing/energy.c

data class EnergyResult(
	// Number of left bit shifts needed to get the physical energy value, i.e, to get the Q0 value
	val rightShifts: Int,

	// Energy value in Q(-[scale_factor])
	val energy: Int
)

//
// Energy(...)
//
// Calculates the energy of a vector
//
// Input:
//      - vector        : Vector which the energy should be calculated on
//      - vector_length : Number of samples in vector
//
// Output:
//      - scale_factor  : Number of left bit shifts needed to get the physical
//                        energy value, i.e, to get the Q0 value
//
// Return value         : Energy value in Q(-[scale_factor])
//
fun Energy(buffer: AudioBuffer): EnergyResult {
	val scaling = GetScalingSquare(buffer, buffer.size)

	var energy = 0
	for (i in 0 until buffer.size) {
		energy += (buffer[i] * buffer[i]) shr scaling
	}

	return EnergyResult(scaling, energy)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/signal_processing/division_operations.c

//
// DivW32W16(...)
//
// Divides a Int [num] by a Int [den].
//
// If [den]==0, (Int)0x7FFFFFFF is returned.
//
// Input:
//      - num       : Numerator
//      - den       : Denominator
//
// Return value     : Result of the division (as a Int), i.e., the
//                    integer part of num/den.
//
fun DivW32W16(num: Int, den: Int) =
	if (den != 0) num / den else Int.MAX_VALUE

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/vad/vad_gmm.c

data class GaussianProbabilityResult(
	// (probability for [input]) = 1 / [std] * exp(-([input] - [mean])^2 / (2 * [std]^2))
	val probability: Int,
	// Input used when updating the model, Q11.
	// [delta] = ([input] - [mean]) / [std]^2.
	val delta: Int
)

val kCompVar = 22005
val kLog2Exp = 5909 // log2(exp(1)) in Q12.

// Calculates the probability for [input], given that [input] comes from a
// normal distribution with mean and standard deviation ([mean], [std]).
//
// Inputs:
//      - input         : input sample in Q4.
//      - mean          : mean input in the statistical model, Q7.
//      - std           : standard deviation, Q7.
//
// Output:
//
//      - delta         : input used when updating the model, Q11.
//                        [delta] = ([input] - [mean]) / [std]^2.
//
// Return:
//   (probability for [input]) =
//    1 / [std] * exp(-([input] - [mean])^2 / (2 * [std]^2));
//---------------------------------------------------------------------------------
// For a normal distribution, the probability of [input] is calculated and
// returned (in Q20). The formula for normal distributed probability is
//
// 1 / s * exp(-(x - m)^2 / (2 * s^2))
//
// where the parameters are given in the following Q domains:
// m = [mean] (Q7)
// s = [std] (Q7)
// x = [input] (Q4)
// in addition to the probability we output [delta] (in Q11) used when updating
// the noise/speech model.
fun GaussianProbability(input: Int, mean: Int, std: Int): GaussianProbabilityResult {
	var tmp16 = 0
	var inv_std = 0
	var inv_std2 = 0
	var exp_value = 0
	var tmp32 = 0

	// Calculate [inv_std] = 1 / s, in Q10.
	// 131072 = 1 in Q17, and ([std] shr 1) is for rounding instead of truncation.
	// Q-domain: Q17 / Q7 = Q10.
	tmp32 = 131072 + (std shr 1)
	inv_std = DivW32W16(tmp32, std)

	// Calculate [inv_std2] = 1 / s^2, in Q14.
	tmp16 = inv_std shr 2 // Q10 -> Q8.
	// Q-domain: (Q8 * Q8) shr 2 = Q14.
	inv_std2 = (tmp16 * tmp16) shr 2

	tmp16 = input shl 3 // Q4 -> Q7
	tmp16 -= mean // Q7 - Q7 = Q7

	// To be used later, when updating noise/speech model.
	// [delta] = (x - m) / s^2, in Q11.
	// Q-domain: (Q14 * Q7) shr 10 = Q11.
	val delta = (inv_std2 * tmp16) shr 10

	// Calculate the exponent [tmp32] = (x - m)^2 / (2 * s^2), in Q10. Replacing
	// division by two with one shift.
	// Q-domain: (Q11 * Q7) shr 8 = Q10.
	tmp32 = (delta * tmp16) shr 9

	// If the exponent is small enough to give a non-zero probability we calculate
	// [exp_value] ~= exp(-(x - m)^2 / (2 * s^2))
	//             ~= exp2(-log2(exp(1)) * [tmp32]).
	if (tmp32 < kCompVar) {
		// Calculate [tmp16] = log2(exp(1)) * [tmp32], in Q10.
		// Q-domain: (Q12 * Q10) shr 12 = Q10.
		tmp16 = (kLog2Exp * tmp32) shr 12
		tmp16 = -tmp16
		exp_value = 0x0400 or (tmp16 and 0x03FF)
		tmp16 = tmp16 xor 0xFFFF
		tmp16 = tmp16 shr 10
		tmp16 += 1
		// Get [exp_value] = exp(-[tmp32]) in Q10.
		exp_value = exp_value shr tmp16
	}

	// Calculate and return (1 / s) * exp(-(x - m)^2 / (2 * s^2)), in Q20.
	// Q-domain: Q10 * Q10 = Q20.
	val probability = inv_std * exp_value
	return GaussianProbabilityResult(probability, delta)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/vad/vad_core.c

// Spectrum Weighting
val kSpectrumWeight = intArrayOf(6, 8, 10, 12, 14, 16).apply { assert(size == kNumChannels) }
val kNoiseUpdateConst: Int = 655 // Q15
val kSpeechUpdateConst: Int = 6554 // Q15
val kBackEta: Int = 154 // Q8
// Minimum difference between the two models, Q5
val kMinimumDifference = intArrayOf(544, 544, 576, 576, 576, 576).apply { assert(size == kNumChannels) }
// Upper limit of mean value for speech model, Q7
val kMaximumSpeech = intArrayOf(11392, 11392, 11520, 11520, 11520, 11520).apply { assert(size == kNumChannels) }
// Minimum value for mean value
val kMinimumMean = intArrayOf(640, 768).apply { assert(size == kNumGaussians) }
// Upper limit of mean value for noise model, Q7
val kMaximumNoise = intArrayOf(9216, 9088, 8960, 8832, 8704, 8576).apply { assert(size == kNumChannels) }
// Start values for the Gaussian models, Q7
// Weights for the two Gaussians for the six channels (noise)
val kNoiseDataWeights = intArrayOf(34, 62, 72, 66, 53, 25, 94, 66, 56, 62, 75, 103).apply { assert(size == kTableSize) }
// Weights for the two Gaussians for the six channels (speech)
val kSpeechDataWeights = intArrayOf(48, 82, 45, 87, 50, 47, 80, 46, 83, 41, 78, 81).apply { assert(size == kTableSize) }
// Means for the two Gaussians for the six channels (noise)
val kNoiseDataMeans = intArrayOf(6738, 4892, 7065, 6715, 6771, 3369, 7646, 3863, 7820, 7266, 5020, 4362).apply { assert(size == kTableSize) }
// Means for the two Gaussians for the six channels (speech)
val kSpeechDataMeans = intArrayOf(8306, 10085, 10078, 11823, 11843, 6309, 9473, 9571, 10879, 7581, 8180, 7483).apply { assert(size == kTableSize) }
// Stds for the two Gaussians for the six channels (noise)
val kNoiseDataStds = intArrayOf(378, 1064, 493, 582, 688, 593, 474, 697, 475, 688, 421, 455).apply { assert(size == kTableSize) }
// Stds for the two Gaussians for the six channels (speech)
val kSpeechDataStds = intArrayOf(555, 505, 567, 524, 585, 1231, 509, 828, 492, 1540, 1079, 850).apply { assert(size == kTableSize) }

// Constants used in GmmProbability().
//
// Maximum number of counted speech (VAD = 1) frames in a row.
val kMaxSpeechFrames: Int = 6
// Minimum standard deviation for both speech and noise.
val kMinStd: Int = 384

// Number of frequency bands (named channels)
val kNumChannels = 6

// Number of Gaussians per channel in the GMM
val kNumGaussians = 2

// Index = gaussian * kNumChannels + channel
val kTableSize = kNumChannels * kNumGaussians

// Minimum energy required to trigger audio signal
val kMinEnergy = 10

/**
 * The VAD operating modes in order of increasing aggressiveness.
 * A more aggressive VAD is more restrictive in reporting speech. Put in other words, the
 * probability of being speech when the VAD returns 1 is increased with increasing mode. As a
 * consequence, the missed detection rate also goes up.
 */
enum class Aggressiveness {
	Quality,
	LowBitrate,
	Aggressive,
	VeryAggressive
}

class VadInstT(aggressiveness: Aggressiveness = Aggressiveness.Quality) {
	// General variables
	var vad: Int = 1 // Speech active (=1)
	// TODO(bjornv): Change to [frame_count].
	var frame_counter: Int = 0
	var over_hang: Int = 0
	var num_of_speech: Int = 0

	// PDF parameters
	var noise_means = kNoiseDataMeans.clone()
	var speech_means = kSpeechDataStds.clone()
	var noise_stds = kNoiseDataStds.clone()
	var speech_stds = kSpeechDataStds.clone()

	// Index vector
	// TODO(bjornv): Change to [age_vector].
	var index_vector = IntArray(16 * kNumChannels) { 0 }

	// Minimum value vector
	var low_value_vector = IntArray(16 * kNumChannels) { 10000 }

	// Splitting filter states
	var upper_state = List(5) { MutableInt(0) }
	var lower_state = List(5) { MutableInt(0) }

	// High pass filter states
	var hp_filter_state = IntArray(4) { 0 }

	// Mean value memory for FindMinimum()
	// TODO(bjornv): Change to [median].
	var mean_value = IntArray(kNumChannels) { 1600 }

	// Thresholds
	val over_hang_max_1: Int = when(aggressiveness) {
		Aggressiveness.Quality, Aggressiveness.LowBitrate -> 8
		Aggressiveness.Aggressive, Aggressiveness.VeryAggressive -> 6
	}
	val over_hang_max_2: Int = when(aggressiveness) {
		Aggressiveness.Quality, Aggressiveness.LowBitrate -> 14
		Aggressiveness.Aggressive, Aggressiveness.VeryAggressive -> 9
	}
	// TODO: Rename to localThreshold?
	val individual: Int = when(aggressiveness) {
		Aggressiveness.Quality -> 24
		Aggressiveness.LowBitrate -> 37
		Aggressiveness.Aggressive -> 82
		Aggressiveness.VeryAggressive -> 94
	}
	// TODO: Rename to globalThreshold?
	val total: Int = when(aggressiveness) {
		Aggressiveness.Quality -> 57
		Aggressiveness.LowBitrate -> 100
		Aggressiveness.Aggressive -> 285
		Aggressiveness.VeryAggressive -> 1100
	}
}

typealias WebRtcVadInst = VadInstT

// Calculates the weighted average w.r.t. number of Gaussians. The [data] are
// updated with an [offset] before averaging.
//
// - data     [i/o] : Data to average.
// - offset   [i]   : An offset to add to each element of [data].
// - weights  [i]   : Weights used for averaging.
//
// returns          : The weighted average.
fun WeightedAverage(data: IntArray, channel: Int, offset: Int, weights: IntArray): Int {
	var result = 0
	for (k in 0 until kNumGaussians) {
		val index = k * kNumChannels + channel
		data[index] += offset
		result += data[index] * weights[index]
	}
	return result
}

// Calculates the probabilities for both speech and background noise using
// Gaussian Mixture Models (GMM). A hypothesis-test is performed to decide which
// type of signal is most probable.
//
// - self           [i/o] : Pointer to VAD instance
// - features       [i]   : Feature vector of length [kNumChannels]
//                          = log10(energy in frequency band)
// - total_power    [i]   : Total power in audio frame.
// - frame_length   [i]   : Number of input samples
//
// - returns              : the VAD decision (0 - noise, 1 - speech).
fun GmmProbability(self: VadInstT, features: List<Int>, total_power: Int, frame_length: Int): Int {
	var vadflag = 0
	var tmp_s16: Int
	var tmp1_s16: Int
	var tmp2_s16: Int
	val deltaN = IntArray(kTableSize)
	val deltaS = IntArray(kTableSize)
	val ngprvec = IntArray(kTableSize) { 0 } // Conditional probability = 0.
	val sgprvec = IntArray(kTableSize) { 0 } // Conditional probability = 0.
	var sum_log_likelihood_ratios = 0
	val noise_probability = IntArray(kNumGaussians)
	val speech_probability = IntArray(kNumGaussians)

	assert(frame_length == 80)

	if (total_power > kMinEnergy) {
		// The signal power of current frame is large enough for processing. The
		// processing consists of two parts:
		// 1) Calculating the likelihood of speech and thereby a VAD decision.
		// 2) Updating the underlying model, w.r.t., the decision made.

		// The detection scheme is an LRT with hypothesis
		// H0: Noise
		// H1: Speech
		//
		// We combine a global LRT with local tests, for each frequency sub-band,
		// here defined as [channel].
		for (channel in 0 until kNumChannels) {
			// For each channel we model the probability with a GMM consisting of
			// [kNumGaussians], with different means and standard deviations depending
			// on H0 or H1.
			var h0_test = 0
			var h1_test = 0
			for (k in 0 until kNumGaussians) {
				val gaussian = channel + k * kNumChannels

				// Probability under H0, that is, probability of frame being noise.
				// Value given in Q27 = Q7 * Q20.
				val pNoise = GaussianProbability(features[channel], self.noise_means[gaussian], self.noise_stds[gaussian])
				deltaN[gaussian] = pNoise.delta
				noise_probability[k] = kNoiseDataWeights[gaussian] * pNoise.probability
				h0_test += noise_probability[k] // Q27

				// Probability under H1, that is, probability of frame being speech.
				// Value given in Q27 = Q7 * Q20.
				val pSpeech = GaussianProbability(features[channel], self.speech_means[gaussian], self.speech_stds[gaussian])
				speech_probability[k] = kSpeechDataWeights[gaussian] * pSpeech.probability
				deltaS[gaussian] = pSpeech.delta
				h1_test += speech_probability[k] // Q27
			}

			// Calculate the log likelihood ratio: log2(Pr{X|H1} / Pr{X|H1}).
			// Approximation:
			// log2(Pr{X|H1} / Pr{X|H1}) = log2(Pr{X|H1}*2^Q) - log2(Pr{X|H1}*2^Q)
			//                           = log2(h1_test) - log2(h0_test)
			//                           = log2(2^(31-shifts_h1)*(1+b1))
			//                             - log2(2^(31-shifts_h0)*(1+b0))
			//                           = shifts_h0 - shifts_h1
			//                             + log2(1+b1) - log2(1+b0)
			//                          ~= shifts_h0 - shifts_h1
			//
			// Note that b0 and b1 are values less than 1, hence, 0 <= log2(1+b0) < 1.
			// Further, b0 and b1 are independent and on the average the two terms cancel.
			val shifts_h0 = if (h0_test != 0) NormW32(h0_test) else 31
			val shifts_h1 = if (h1_test != 0) NormW32(h1_test) else 31
			val log_likelihood_ratio = shifts_h0 - shifts_h1

			// Update [sum_log_likelihood_ratios] with spectrum weighting. This is
			// used for the global VAD decision.
			sum_log_likelihood_ratios += log_likelihood_ratio * kSpectrumWeight[channel]

			// Local VAD decision.
			if ((log_likelihood_ratio * 4) > self.individual) {
				vadflag = 1
			}

			// TODO(bjornv): The conditional probabilities below are applied on the
			// hard coded number of Gaussians set to two. Find a way to generalize.
			// Calculate local noise probabilities used later when updating the GMM.
			val h0 = h0_test shr 12 // Q15
			if (h0 > 0) {
				// High probability of noise. Assign conditional probabilities for each
				// Gaussian in the GMM.
				val tmp = (noise_probability[0] and 0xFFFFF000u.toInt()) shl 2 // Q29
				ngprvec[channel] = DivW32W16(tmp, h0) // Q14
				ngprvec[channel + kNumChannels] = 16384 - ngprvec[channel]
			} else {
				// Low noise probability. Assign conditional probability 1 to the first
				// Gaussian and 0 to the rest (which is already set at initialization).
				ngprvec[channel] = 16384
			}

			// Calculate local speech probabilities used later when updating the GMM.
			val h1 = (h1_test shr 12) // Q15
			if (h1 > 0) {
				// High probability of speech. Assign conditional probabilities for each
				// Gaussian in the GMM. Otherwise use the initialized values, i.e., 0.
				val tmp = (speech_probability[0] and 0xFFFFF000u.toInt()) shl 2 // Q29
				sgprvec[channel] = DivW32W16(tmp, h1) // Q14
				sgprvec[channel + kNumChannels] = 16384 - sgprvec[channel]
			}
		}

		// Make a global VAD decision.
		vadflag = vadflag or (if (sum_log_likelihood_ratios >= self.total) 1 else 0)

		// Update the model parameters.
		var maxspe = 12800
		for (channel in 0 until kNumChannels) {
			// Get minimum value in past which is used for long term correction in Q4.
			val feature_minimum = FindMinimum(self, features[channel], channel)

			// Compute the "global" mean, that is the sum of the two means weighted.
			var noise_global_mean = WeightedAverage(self.noise_means, channel, 0, kNoiseDataWeights)
			tmp1_s16 = noise_global_mean shr 6 // Q8

			for (k in 0 until kNumGaussians) {
				val gaussian = channel + k * kNumChannels

				val nmk = self.noise_means[gaussian]
				val smk = self.speech_means[gaussian]
				var nsk = self.noise_stds[gaussian]
				var ssk = self.speech_stds[gaussian]

				// Update noise mean vector if the frame consists of noise only.
				var nmk2 = nmk
				if (vadflag == 0) {
					// deltaN = (x-mu)/sigma^2
					// ngprvec[k] = |noise_probability[k]| /
					//   (|noise_probability[0]| + |noise_probability[1]|)

					// (Q14 * Q11 shr 11) = Q14.
					val delt = (ngprvec[gaussian] * deltaN[gaussian]) shr 11
					// Q7 + (Q14 * Q15 shr 22) = Q7.
					nmk2 = nmk + (delt * kNoiseUpdateConst) shr 22
				}

				// Long term correction of the noise mean.
				// Q8 - Q8 = Q8.
				val ndelt = (feature_minimum shl 4) - tmp1_s16
				// Q7 + (Q8 * Q8) shr 9 = Q7.
				var nmk3 = nmk2 + ((ndelt * kBackEta) shr 9)

				// Control that the noise mean does not drift to much.
				tmp_s16 = (k + 5) shl 7
				if (nmk3 < tmp_s16) {
					nmk3 = tmp_s16
				}
				tmp_s16 = (72 + k - channel) shl 7
				if (nmk3 > tmp_s16) {
					nmk3 = tmp_s16
				}
				self.noise_means[gaussian] = nmk3

				if (vadflag != 0) {
					// Update speech mean vector:
					// [deltaS] = (x-mu)/sigma^2
					// sgprvec[k] = |speech_probability[k]| /
					//   (|speech_probability[0]| + |speech_probability[1]|)

					// (Q14 * Q11) shr 11 = Q14.
					val delt = (sgprvec[gaussian] * deltaS[gaussian]) shr 11
					// Q14 * Q15 shr 21 = Q8.
					tmp_s16 = (delt * kSpeechUpdateConst) shr 21
					// Q7 + (Q8 shr 1) = Q7. With rounding.
					var smk2 = smk + ((tmp_s16 + 1) shr 1)

					// Control that the speech mean does not drift to much.
					val maxmu = maxspe + 640
					if (smk2 < kMinimumMean[k]) {
						smk2 = kMinimumMean[k]
					}
					if (smk2 > maxmu) {
						smk2 = maxmu
					}
					self.speech_means[gaussian] = smk2 // Q7.

					// (Q7 shr 3) = Q4. With rounding.
					tmp_s16 = ((smk + 4) shr 3)

					tmp_s16 = features[channel] - tmp_s16 // Q4
					// (Q11 * Q4 shr 3) = Q12.
					var tmp1_s32 = (deltaS[gaussian] * tmp_s16) shr 3
					var tmp2_s32 = tmp1_s32 - 4096
					tmp_s16 = sgprvec[gaussian] shr 2
					// (Q14 shr 2) * Q12 = Q24.
					tmp1_s32 = tmp_s16 * tmp2_s32

					tmp2_s32 = tmp1_s32 shr 4 // Q20

					// 0.1 * Q20 / Q7 = Q13.
					if (tmp2_s32 > 0) {
						tmp_s16 = DivW32W16(tmp2_s32, ssk * 10)
					} else {
						tmp_s16 = DivW32W16(-tmp2_s32, ssk * 10)
						tmp_s16 = -tmp_s16
					}
					// Divide by 4 giving an update factor of 0.025 (= 0.1 / 4).
					// Note that division by 4 equals shift by 2, hence,
					// (Q13 shr 8) = (Q13 shr 6) / 4 = Q7.
					tmp_s16 += 128 // Rounding.
					ssk += (tmp_s16 shr 8)
					if (ssk < kMinStd) {
						ssk = kMinStd
					}
					self.speech_stds[gaussian] = ssk
				} else {
					// Update GMM variance vectors.
					// deltaN * (features[channel] - nmk) - 1
					// Q4 - (Q7 shr 3) = Q4.
					tmp_s16 = features[channel] - (nmk shr 3)
					// (Q11 * Q4 shr 3) = Q12.
					var tmp1_s32 = (deltaN[gaussian] * tmp_s16) shr 3
					tmp1_s32 -= 4096

					// (Q14 shr 2) * Q12 = Q24.
					tmp_s16 = (ngprvec[gaussian] + 2) shr 2
					val tmp2_s32 = tmp_s16 * tmp1_s32
					// Q20  * approx 0.001 (2^-10=0.0009766), hence,
					// (Q24 shr 14) = (Q24 shr 4) / 2^10 = Q20.
					tmp1_s32 = tmp2_s32 shr 14

					// Q20 / Q7 = Q13.
					if (tmp1_s32 > 0) {
						tmp_s16 = DivW32W16(tmp1_s32, nsk)
					} else {
						tmp_s16 = DivW32W16(-tmp1_s32, nsk)
						tmp_s16 = -tmp_s16
					}
					tmp_s16 += 32 // Rounding
					nsk += tmp_s16 shr 6 // Q13 shr 6 = Q7.
					if (nsk < kMinStd) {
						nsk = kMinStd
					}
					self.noise_stds[gaussian] = nsk
				}
			}

			// Separate models if they are too close.
			// [noise_global_mean] in Q14 (= Q7 * Q7).
			noise_global_mean = WeightedAverage(self.noise_means, channel, 0, kNoiseDataWeights)

			// [speech_global_mean] in Q14 (= Q7 * Q7).
			var speech_global_mean = WeightedAverage(self.speech_means, channel, 0, kSpeechDataWeights)

			// [diff] = "global" speech mean - "global" noise mean.
			// (Q14 shr 9) - (Q14 shr 9) = Q5.
			val diff = (speech_global_mean shr 9) - (noise_global_mean shr 9)
			if (diff < kMinimumDifference[channel]) {
				tmp_s16 = kMinimumDifference[channel] - diff

				// [tmp1_s16] = ~0.8 * (kMinimumDifference - diff) in Q7.
				// [tmp2_s16] = ~0.2 * (kMinimumDifference - diff) in Q7.
				tmp1_s16 = (13 * tmp_s16) shr 2
				tmp2_s16 = (3 * tmp_s16) shr 2

				// Move Gaussian means for speech model by [tmp1_s16] and update
				// [speech_global_mean]. Note that |self.speech_means[channel]| is
				// changed after the call.
				speech_global_mean = WeightedAverage(self.speech_means, channel, tmp1_s16, kSpeechDataWeights)

				// Move Gaussian means for noise model by -[tmp2_s16] and update
				// [noise_global_mean]. Note that |self.noise_means[channel]| is
				// changed after the call.
				noise_global_mean = WeightedAverage(self.noise_means, channel, -tmp2_s16, kNoiseDataWeights)
			}

			// Control that the speech & noise means do not drift to much.
			maxspe = kMaximumSpeech[channel]
			tmp2_s16 = speech_global_mean shr 7
			if (tmp2_s16 > maxspe) {
				// Upper limit of speech model.
				tmp2_s16 -= maxspe

				for (k in 0 until kNumGaussians) {
					self.speech_means[channel + k * kNumChannels] -= tmp2_s16
				}
			}

			tmp2_s16 = noise_global_mean shr 7
			if (tmp2_s16 > kMaximumNoise[channel]) {
				tmp2_s16 -= kMaximumNoise[channel]

				for (k in 0 until kNumGaussians) {
					self.noise_means[channel + k * kNumChannels] -= tmp2_s16
				}
			}
		}
		self.frame_counter++
	}

	// Smooth with respect to transition hysteresis.
	if (vadflag == 0) {
		if (self.over_hang > 0) {
			vadflag = 2 + self.over_hang
			self.over_hang--
		}
		self.num_of_speech = 0
	} else {
		self.num_of_speech++
		if (self.num_of_speech > kMaxSpeechFrames) {
			self.num_of_speech = kMaxSpeechFrames
			self.over_hang = self.over_hang_max_2
		} else {
			self.over_hang = self.over_hang_max_1
		}
	}
	return vadflag
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/vad/vad_sp.c

val kSmoothingDown = 6553 // 0.2 in Q15.
val kSmoothingUp = 32439 // 0.99 in Q15.

// Updates and returns the smoothed feature minimum. As minimum we use the
// median of the five smallest feature values in a 100 frames long window.
// As long as |handle->frame_counter| is zero, that is, we haven't received any
// "valid" data, FindMinimum() outputs the default value of 1600.
//
// Inputs:
//      - feature_value : New feature value to update with.
//      - channel       : Channel number.
//
// Input & Output:
//      - handle        : State information of the VAD.
//
// Returns:
//                      : Smoothed minimum value for a moving window.
// Inserts [feature_value] into [low_value_vector], if it is one of the 16
// smallest values the last 100 frames. Then calculates and returns the median
// of the five smallest values.
fun FindMinimum(self: VadInstT, feature_value: Int, channel: Int): Int {
	var position = -1
	var current_median = 1600
	var alpha = 0
	var tmp32 = 0
	val offset = channel * 16

	// Accessor for the age of each value of the [channel]
	val age = object {
		inline operator fun get(i: Int) = self.index_vector[offset + i]
		inline operator fun set(i: Int, value: Int) { self.index_vector[offset + i] = value }
	}

	// Accessor for the 16 minimum values of the [channel]
	val smallest_values = object {
		inline operator fun get(i: Int) = self.low_value_vector[offset + i]
		inline operator fun set(i: Int, value: Int) { self.low_value_vector[offset + i] = value }
	}

	assert(channel < kNumChannels)

	// Each value in [smallest_values] is getting 1 loop older. Update [age], and
	// remove old values.
	for (i in 0 until 16) {
		if (age[i] != 100) {
			age[i]++
		} else {
			// Too old value. Remove from memory and shift larger values downwards.
			for (j in i until 16) {
				smallest_values[j] = smallest_values[j + 1]
				age[j] = age[j + 1]
			}
			age[15] = 101
			smallest_values[15] = 10000
		}
	}

	// Check if [feature_value] is smaller than any of the values in
	// [smallest_values]. If so, find the [position] where to insert the new value
	// ([feature_value]).
	if (feature_value < smallest_values[7]) {
		if (feature_value < smallest_values[3]) {
			if (feature_value < smallest_values[1]) {
				if (feature_value < smallest_values[0]) {
					position = 0
				} else {
					position = 1
				}
			} else if (feature_value < smallest_values[2]) {
				position = 2
			} else {
				position = 3
			}
		} else if (feature_value < smallest_values[5]) {
			if (feature_value < smallest_values[4]) {
				position = 4
			} else {
				position = 5
			}
		} else if (feature_value < smallest_values[6]) {
			position = 6
		} else {
			position = 7
		}
	} else if (feature_value < smallest_values[15]) {
		if (feature_value < smallest_values[11]) {
			if (feature_value < smallest_values[9]) {
				if (feature_value < smallest_values[8]) {
					position = 8
				} else {
					position = 9
				}
			} else if (feature_value < smallest_values[10]) {
				position = 10
			} else {
				position = 11
			}
		} else if (feature_value < smallest_values[13]) {
			if (feature_value < smallest_values[12]) {
				position = 12
			} else {
				position = 13
			}
		} else if (feature_value < smallest_values[14]) {
			position = 14
		} else {
			position = 15
		}
	}

	// If we have detected a new small value, insert it at the correct position
	// and shift larger values up.
	if (position > -1) {
		for (i in 15 downTo position + 1) {
			smallest_values[i] = smallest_values[i - 1]
			age[i] = age[i - 1]
		}
		smallest_values[position] = feature_value
		age[position] = 1
	}

	// Get [current_median].
	if (self.frame_counter > 2) {
		current_median = smallest_values[2]
	} else if (self.frame_counter > 0) {
		current_median = smallest_values[0]
	}

	// Smooth the median value.
	if (self.frame_counter > 0) {
		if (current_median < self.mean_value[channel]) {
			alpha = kSmoothingDown // 0.2 in Q15.
		} else {
			alpha = kSmoothingUp // 0.99 in Q15.
		}
	}
	tmp32 = (alpha + 1) * self.mean_value[channel]
	tmp32 += (WEBRTC_SPL_WORD16_MAX - alpha) * current_median
	tmp32 += 16384
	self.mean_value[channel] = tmp32 shr 15

	return self.mean_value[channel]
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/vad/vad_filterbank.c

// Constants used in LogOfEnergy().
val kLogConst = 24660 // 160*log10(2) in Q9.
val kLogEnergyIntPart = 14336 // 14 in Q10

// Coefficients used by HighPassFilter, Q14.
val kHpZeroCoefs = intArrayOf(6631, -13262, 6631)
val kHpPoleCoefs = intArrayOf(16384, -7756, 5620)

// Allpass filter coefficients, upper and lower, in Q15.
// Upper: 0.64, Lower: 0.17
val kUpperAllPassCoefsQ15 = 20972
val kLowerAllPassCoefsQ15 = 5571

// Adjustment for division with two in SplitFilter.
val kOffsetVector = intArrayOf(368, 368, 272, 176, 176, 176)

// High pass filtering, with a cut-off frequency at 80 Hz, if the [data_in] is
// sampled at 500 Hz.
//
// - data_in      [i]   : Input audio data sampled at 500 Hz.
// - data_length  [i]   : Length of input and output data.
// - filter_state [i/o] : State of the filter.
// - data_out     [o]   : Output audio data in the frequency interval
//                        80 - 250 Hz.
fun HighPassFilter(input: AudioBuffer, filter_state: IntArray): AudioBuffer {
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
		var tmp32 = kHpZeroCoefs[0] * input[i]
		tmp32 += kHpZeroCoefs[1] * filter_state[0]
		tmp32 += kHpZeroCoefs[2] * filter_state[1]
		filter_state[1] = filter_state[0]
		filter_state[0] = input[i].toInt()

		// All-pole section (filter coefficients in Q14).
		tmp32 -= kHpPoleCoefs[1] * filter_state[2]
		tmp32 -= kHpPoleCoefs[2] * filter_state[3]
		filter_state[3] = filter_state[2]
		filter_state[2] = tmp32 shr 14
		result[i] = filter_state[2].toShort()
	}

	return AudioBuffer(result)
}

// All pass filtering of [data_in], used before splitting the signal into two
// frequency bands (low pass vs high pass).
// Note that [data_in] and [data_out] can NOT correspond to the same address.
//
// - data_in            [i]   : Input audio signal given in Q0.
// - data_length        [i]   : Length of input and output data.
// - filter_coefficient [i]   : Given in Q15.
// - filter_state       [i/o] : State of the filter given in Q(-1).
// - data_out           [o]   : Output audio signal given in Q(-1).
fun AllPassFilter(input: AudioBuffer, filter_coefficient: Int, filter_state: MutableInt): AudioBuffer {
	// The filter can only cause overflow (in the w16 output variable)
	// if more than 4 consecutive input numbers are of maximum value and
	// has the the same sign as the impulse responses first taps.
	// First 6 taps of the impulse response:
	// 0.6399 0.5905 -0.3779 0.2418 -0.1547 0.0990

	val result = SampleArray(input.size / 2)
	var state32 = filter_state.toInt() * (1 shl 16) // Q15
	for (i in 0 until input.size step 2) {
		val tmp32 = state32 + filter_coefficient * input[i]
		val tmp16 = tmp32 shr 16 // Q(-1)
		result[i / 2] = tmp16.toShort()
		state32 = (input[i] * (1 shl 14)) - filter_coefficient * tmp16 // Q14
		state32 *= 2 // Q15.
	}
	filter_state.setValue(state32 shr 16) // Q(-1)

	return AudioBuffer(result)
}

data class SplitFilterResult(
	val highPassData: AudioBuffer,
	val lowPassData: AudioBuffer
)

// Splits [data_in] into [hp_data_out] and [lp_data_out] corresponding to
// an upper (high pass) part and a lower (low pass) part respectively.
//
// - data_in      [i]   : Input audio data to be split into two frequency bands.
// - data_length  [i]   : Length of [data_in].
// - upper_state  [i/o] : State of the upper filter, given in Q(-1).
// - lower_state  [i/o] : State of the lower filter, given in Q(-1).
// - hp_data_out  [o]   : Output audio data of the upper half of the spectrum.
//                        The length is [data_length] / 2.
// - lp_data_out  [o]   : Output audio data of the lower half of the spectrum.
//                        The length is [data_length] / 2.
fun SplitFilter(input: AudioBuffer, upper_state: MutableInt, lower_state: MutableInt): SplitFilterResult {
	val resultSize = input.size / 2 // Downsampling by 2

	// All-pass filtering upper branch.
	val tempHighPass = AllPassFilter(input, kUpperAllPassCoefsQ15, upper_state)
	assert(tempHighPass.size == resultSize)

	// All-pass filtering lower branch.
	val tempLowPass = AllPassFilter(AudioBuffer(input, 1), kLowerAllPassCoefsQ15, lower_state)
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

// Calculates the energy of [data_in] in dB, and also updates an overall
// [total_energy] if necessary.
//
// - data_in      [i]   : Input audio data for energy calculation.
// - data_length  [i]   : Length of input data.
// - offset       [i]   : Offset value added to [log_energy].
// - total_energy [i/o] : An external energy updated with the energy of
//                        [data_in].
//                        NOTE: [total_energy] is only updated if
//                        [total_energy] <= [kMinEnergy].
// - log_energy   [o]   : 10 * log10("energy of [data_in]") given in Q4.
fun LogOfEnergy(input: AudioBuffer, offset: Int, total_energy: MutableInt): Int {
	assert(input.size > 0)

	val energyResult = Energy(input)
	// [tot_rshifts] accumulates the number of right shifts performed on [energy].
	var tot_rshifts = energyResult.rightShifts
	// The [energy] will be normalized to 15 bits. We use unsigned integer because
	// we eventually will mask out the fractional part.
	var energy = energyResult.energy.toUInt()

	if (energy == 0u) {
		return offset
	}

	// By construction, normalizing to 15 bits is equivalent with 17 leading
	// zeros of an unsigned 32 bit value.
	val normalizing_rshifts = 17 - NormU32(energy)
	// In a 15 bit representation the leading bit is 2^14. log2(2^14) in Q10 is
	// (14 shl 10), which is what we initialize [log2_energy] with. For a more
	// detailed derivations, see below.
	var log2_energy = kLogEnergyIntPart

	tot_rshifts += normalizing_rshifts
	// Normalize [energy] to 15 bits.
	// [tot_rshifts] is now the total number of right shifts performed on
	// [energy] after normalization. This means that [energy] is in
	// Q(-tot_rshifts).
	energy = if (normalizing_rshifts < 0)
		energy shl -normalizing_rshifts
	else
		energy shr normalizing_rshifts

	// Calculate the energy of [data_in] in dB, in Q4.
	//
	// 10 * log10("true energy") in Q4 = 2^4 * 10 * log10("true energy") =
	// 160 * log10([energy] * 2^[tot_rshifts]) =
	// 160 * log10(2) * log2([energy] * 2^[tot_rshifts]) =
	// 160 * log10(2) * (log2([energy]) + log2(2^[tot_rshifts])) =
	// (160 * log10(2)) * (log2([energy]) + [tot_rshifts]) =
	// [kLogConst] * ([log2_energy] + [tot_rshifts])
	//
	// We know by construction that [energy] is normalized to 15 bits. Hence,
	// [energy] = 2^14 + frac_Q15, where frac_Q15 is a fractional part in Q15.
	// Further, we'd like [log2_energy] in Q10
	// log2([energy]) in Q10 = 2^10 * log2(2^14 + frac_Q15) =
	// 2^10 * log2(2^14 * (1 + frac_Q15 * 2^-14)) =
	// 2^10 * (14 + log2(1 + frac_Q15 * 2^-14)) ~=
	// (14 shl 10) + 2^10 * (frac_Q15 * 2^-14) =
	// (14 shl 10) + (frac_Q15 * 2^-4) = (14 shl 10) + (frac_Q15 shr 4)
	//
	// Note that frac_Q15 = ([energy] & 0x00003FFF)

	// Calculate and add the fractional part to [log2_energy].
	log2_energy += ((energy and 0x00003FFFu) shr 4).toInt()

	// [kLogConst] is in Q9, [log2_energy] in Q10 and [tot_rshifts] in Q0.
	// Note that we in our derivation above have accounted for an output in Q4.
	var log_energy = (((kLogConst * log2_energy) shr 19) + (tot_rshifts * kLogConst) shr 9)

	if (log_energy < 0) {
		log_energy = 0
	}

	log_energy += offset

	// Update the approximate [total_energy] with the energy of [data_in], if
	// [total_energy] has not exceeded [kMinEnergy]. [total_energy] is used as an
	// energy indicator in GmmProbability() in vad_core.c.
	if (total_energy.toInt() <= kMinEnergy) {
		if (tot_rshifts >= 0) {
			// We know by construction that the [energy] > [kMinEnergy] in Q0, so add
			// an arbitrary value such that [total_energy] exceeds [kMinEnergy].
			total_energy.add(kMinEnergy + 1)
		} else {
			// By construction [energy] is represented by 15 bits, hence any number of
			// right shifted [energy] will fit in an Int. In addition, adding the
			// value to [total_energy] is wrap around safe as long as
			// [kMinEnergy] < 8192.
			total_energy.add((energy shr -tot_rshifts).toInt()) // Q0.
		}
	}

	return log_energy
}

data class FeatureResult(
	// 10 * log10(energy in each frequency band), Q4
	val features: List<Int>,
	// Total energy of the signal
	// NOTE: This value is not exact. It is only used in a comparison.
	val totalEnergy: Int
)

// Takes [data_length] samples of [data_in] and calculates the logarithm of the
// energy of each of the [kNumChannels] = 6 frequency bands used by the VAD:
//        80 Hz - 250 Hz
//        250 Hz - 500 Hz
//        500 Hz - 1000 Hz
//        1000 Hz - 2000 Hz
//        2000 Hz - 3000 Hz
//        3000 Hz - 4000 Hz
//
// The values are given in Q4 and written to [features]. Further, an approximate
// overall energy is returned. The return value is used in
// GmmProbability() as a signal indicator, hence it is arbitrary above
// the threshold [kMinEnergy].
//
// - self         [i/o] : State information of the VAD.
// - data_in      [i]   : Input audio data, for feature extraction.
// - data_length  [i]   : Audio data size, in number of samples.
// - features     [o]   : 10 * log10(energy in each frequency band), Q4.
// - returns            : Total energy of the signal (NOTE! This value is not
//                        exact. It is only used in a comparison.)
fun CalculateFeatures(self: VadInstT, input: AudioBuffer): FeatureResult {
	assert(input.size == 80)

	// Split at 2000 Hz and downsample.
	var frequency_band = 0
	val `0 to 4000 Hz` = input
	val (`2000 to 4000 Hz`, `0 to 2000 Hz`) =
		SplitFilter(`0 to 4000 Hz`, self.upper_state[frequency_band], self.lower_state[frequency_band])

	// For the upper band (2000 to 4000 Hz) split at 3000 Hz and downsample.
	frequency_band = 1
	val (`3000 to 4000 Hz`, `2000 to 3000 Hz`) =
		SplitFilter(`2000 to 4000 Hz`, self.upper_state[frequency_band], self.lower_state[frequency_band])

	// For the lower band (0 to 2000 Hz) split at 1000 Hz and downsample.
	frequency_band = 2
	val (`1000 to 2000 Hz`, `0 to 1000 Hz`) =
		SplitFilter(`0 to 2000 Hz`, self.upper_state[frequency_band], self.lower_state[frequency_band])

	// For the lower band (0 to 1000 Hz) split at 500 Hz and downsample.
	frequency_band = 3
	val (`500 to 1000 Hz`, `0 to 500 Hz`) =
		SplitFilter(`0 to 1000 Hz`, self.upper_state[frequency_band], self.lower_state[frequency_band])

	// For the lower band (0 t0 500 Hz) split at 250 Hz and downsample.
	frequency_band = 4
	val (`250 to 500 Hz`, `0 to 250 Hz`) =
		SplitFilter(`0 to 500 Hz`, self.upper_state[frequency_band], self.lower_state[frequency_band])

	// Remove 0 to 80 Hz by high pass filtering the lower band.
	val `80 to 250 Hz` = HighPassFilter(`0 to 250 Hz`, self.hp_filter_state)

	val total_energy = MutableInt(0)
	val `energy in 3000 to 4000 Hz` = LogOfEnergy(`3000 to 4000 Hz`, kOffsetVector[5], total_energy)
	val `energy in 2000 to 3000 Hz` = LogOfEnergy(`2000 to 3000 Hz`, kOffsetVector[4], total_energy)
	val `energy in 1000 to 2000 Hz` = LogOfEnergy(`1000 to 2000 Hz`, kOffsetVector[3], total_energy)
	val `energy in 500 to 1000 Hz` = LogOfEnergy(`500 to 1000 Hz`, kOffsetVector[2], total_energy)
	val `energy in 250 to 500 Hz` = LogOfEnergy(`250 to 500 Hz`, kOffsetVector[1], total_energy)
	val `energy in 50 to 250 Hz` = LogOfEnergy(`80 to 250 Hz`, kOffsetVector[0], total_energy)

	val features = listOf(
		`energy in 50 to 250 Hz`,
		`energy in 250 to 500 Hz`,
		`energy in 500 to 1000 Hz`,
		`energy in 1000 to 2000 Hz`,
		`energy in 2000 to 3000 Hz`,
		`energy in 3000 to 4000 Hz`
	)
	assert(features.size == kNumChannels)
	return FeatureResult(features, total_energy.toInt())
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// webrtc/common_audio/vad/webrtc_vad.c

// This function was moved from vad_core.c.
/****************************************************************************
 * CalcVad48khz(...)
 * CalcVad32khz(...)
 * CalcVad16khz(...)
 * CalcVad8khz(...)
 *
 * Calculate probability for active speech and make VAD decision.
 *
 * Input:
 *      - inst          : Instance that should be initialized
 *      - speech_frame  : Input speech frame
 *      - frame_length  : Number of input samples
 *
 * Output:
 *      - inst          : Updated filter states etc.
 *
 * Return value         : VAD decision
 *                        0 - No active speech
 *                        1-6 - Active speech
 */
fun CalcVad8khz(inst: VadInstT, speech_frame: AudioBuffer): Int {
	// Get power in the bands
	val (features, totalEnergy) = CalculateFeatures(inst, speech_frame)

	// Make a VAD
	inst.vad = GmmProbability(inst, features, totalEnergy, speech_frame.size)

	return inst.vad
}

// Calculates a VAD decision for the [audio_frame]. For valid sampling rates
// frame lengths, see the description of ValidRatesAndFrameLengths().
//
// - handle       [i/o] : VAD Instance. Needs to be initialized by
//                        InitVadInst() before call.
// - fs           [i]   : Sampling frequency (Hz): 8000, 16000, or 32000
// - audio_frame  [i]   : Audio frame buffer.
// - frame_length [i]   : Length of audio frame buffer in number of samples.
//
// returns              : 1 - (Active Voice),
//                        0 - (Non-active Voice),
//                       -1 - (Error)
fun ProcessVad(self: VadInstT, fs: Int, audio_frame: AudioBuffer): Boolean {
	assert(fs == 8000)

	val vad = CalcVad8khz(self, audio_frame)
	return vad != 0
}
