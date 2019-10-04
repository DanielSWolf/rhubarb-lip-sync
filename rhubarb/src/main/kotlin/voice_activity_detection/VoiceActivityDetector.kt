package voice_activity_detection

import AudioBuffer
import org.apache.commons.lang3.mutable.MutableInt

private const val COMP_VAR = 22005
private const val LOG2EXP = 5909 // log2(exp(1)) in Q12

private val SPECTRUM_WEIGHT = intArrayOf(6, 8, 10, 12, 14, 16)
	.apply { assert(size == CHANNEL_COUNT) }

private const val NOISE_UPDATE_CONST: Int = 655 // Q15

private const val SPEECH_UPDATE_CONST: Int = 6554 // Q15

private const val BACK_ETA: Int = 154 // Q8

/** Upper limit of mean value for speech model, Q7 */
private val MAXIMUM_SPEECH = intArrayOf(11392, 11392, 11520, 11520, 11520, 11520)
	.apply { assert(size == CHANNEL_COUNT) }

/** Minimum difference between the two models, Q5 */
private val MINIMUM_DIFFERENCE = intArrayOf(544, 544, 576, 576, 576, 576)
	.apply { assert(size == CHANNEL_COUNT) }

/** Minimum value for mean value */
private val MINIMUM_MEAN = intArrayOf(640, 768)
	.apply { assert(size == GAUSSIAN_COUNT) }

/** Upper limit of mean value for noise model, Q7 */
private val MAXIMUM_NOISE = intArrayOf(9216, 9088, 8960, 8832, 8704, 8576)
	.apply { assert(size == CHANNEL_COUNT) }

/** Weights for the two Gaussians for the six channels (noise) */
private val NOISE_DATA_WEIGHTS = intArrayOf(34, 62, 72, 66, 53, 25, 94, 66, 56, 62, 75, 103)
	.apply { assert(size == TABLE_SIZE) }

/** Weights for the two Gaussians for the six channels (speech) */
private val SPEECH_DATA_WEIGHTS = intArrayOf(48, 82, 45, 87, 50, 47, 80, 46, 83, 41, 78, 81)
	.apply { assert(size == TABLE_SIZE) }

/** Means for the two Gaussians for the six channels (noise) */
private val NOISE_DATA_MEANS = intArrayOf(6738, 4892, 7065, 6715, 6771, 3369, 7646, 3863, 7820, 7266, 5020, 4362)
	.apply { assert(size == TABLE_SIZE) }

/** Means for the two Gaussians for the six channels (speech) */
private val SPEECH_DATA_MEANS = intArrayOf(8306, 10085, 10078, 11823, 11843, 6309, 9473, 9571, 10879, 7581, 8180, 7483)
	.apply { assert(size == TABLE_SIZE) }

/** Stds for the two Gaussians for the six channels (noise) */
private val NOISE_DATA_STDS = intArrayOf(378, 1064, 493, 582, 688, 593, 474, 697, 475, 688, 421, 455)
	.apply { assert(size == TABLE_SIZE) }

/** Stds for the two Gaussians for the six channels (speech) */
private val SPEECH_DATA_STDS = intArrayOf(555, 505, 567, 524, 585, 1231, 509, 828, 492, 1540, 1079, 850)
	.apply { assert(size == TABLE_SIZE) }

/** Maximum number of counted speech (VAD = 1) frames in a row */
private const val MAX_SPEECH_FRAMES = 6

/** Minimum standard deviation for both speech and noise */
private const val MIN_STD = 384

/** Number of frequency bands (named channels) */
private const val CHANNEL_COUNT = 6

/** Number of Gaussians per channel in the GMM */
private const val GAUSSIAN_COUNT = 2

/**
 * Size of a table containing one value per channel and Gaussian.
 * Indexed as gaussian * CHANNEL_COUNT + channel.
 */
private const val TABLE_SIZE = CHANNEL_COUNT * GAUSSIAN_COUNT

// Adjustment for division with two in splitFilter.
private val SPLIT_FILTER_OFFSETS = intArrayOf(368, 368, 272, 176, 176, 176)

private const val SMOOTHING_DOWN = 6553 // 0.2 in Q15.
private const val SMOOTHING_UP = 32439 // 0.99 in Q15.

/** Performs a safe integer division, returning [Int.MAX_VALUE] if [denominator] = 0. */
private infix fun Int.safeDiv(denominator: Int) = if (denominator != 0) this / denominator else Int.MAX_VALUE

private data class GaussianProbabilityResult(
	/** (probability for input) = 1 / std * exp(-(input - mean)^2 / (2 * std^2)) */
	val probability: Int,

	/**
	 * Input used when updating the model, Q11.
	 * delta = (input - mean) / std^2.
	 */
	val delta: Int
)

/**
 * Calculates the probability for [input], given that [input] comes from a normal distribution with
 * mean [mean] and standard deviation [std].
 *
 * @param [input] Input sample in Q4.
 * @param [mean] Mean input in the statistical model, Q7.
 * @param [std] Standard deviation, Q7.
 */
private fun getGaussianProbability(input: Int, mean: Int, std: Int): GaussianProbabilityResult {
	var tmp16: Int
	var expValue = 0

	// Calculate invStd = 1 / s, in Q10.
	// 131072 = 1 in Q17, and (std shr 1) is for rounding instead of truncation.
	// Q-domain: Q17 / Q7 = Q10
	var tmp32 = 131072 + (std shr 1)
	val invStd = tmp32 safeDiv std

	// Calculate inv_std2 = 1 / s^2, in Q14
	tmp16 = invStd shr 2 // Q10 -> Q8.
	// Q-domain: (Q8 * Q8) shr 2 = Q14
	val invStd2 = (tmp16 * tmp16) shr 2

	tmp16 = input shl 3 // Q4 -> Q7
	tmp16 -= mean // Q7 - Q7 = Q7

	// To be used later, when updating noise/speech model.
	// delta = (x - m) / s^2, in Q11.
	// Q-domain: (Q14 * Q7) shr 10 = Q11
	val delta = (invStd2 * tmp16) shr 10

	// Calculate the exponent tmp32 = (x - m)^2 / (2 * s^2), in Q10.
	// Replacing division by two with one shift.
	// Q-domain: (Q11 * Q7) shr 8 = Q10.
	tmp32 = (delta * tmp16) shr 9

	// If the exponent is small enough to give a non-zero probability, we calculate
	// exp_value ~= exp(-(x - m)^2 / (2 * s^2))
	//           ~= exp2(-log2(exp(1)) * tmp32)
	if (tmp32 < COMP_VAR) {
		// Calculate tmp16 = log2(exp(1)) * tmp32, in Q10.
		// Q-domain: (Q12 * Q10) shr 12 = Q10.
		tmp16 = (LOG2EXP * tmp32) shr 12
		tmp16 = -tmp16
		expValue = 0x0400 or (tmp16 and 0x03FF)
		tmp16 = tmp16 xor 0xFFFF
		tmp16 = tmp16 shr 10
		tmp16 += 1
		// Get expValue = exp(-tmp32) in Q10.
		expValue = expValue shr tmp16
	}

	// Calculate and return (1 / s) * exp(-(x - m)^2 / (2 * s^2)), in Q20.
	// Q-domain: Q10 * Q10 = Q20.
	val probability = invStd * expValue
	return GaussianProbabilityResult(probability, delta)
}

/**
 * Calculates the weighted average with regard to number of Gaussians.
 * CAUTION: Modifies [data] by adding the specified offset to each element.
 *
 * @param[data] Data to average.
 * @param[offset] An offset to add to each element of [data].
 * @param[weights] Weights used for averaging.
 * @return The weighted average.
 */
private fun getWeightedAverage(data: IntArray, channel: Int, offset: Int, weights: IntArray): Int {
	var result = 0
	for (k in 0 until GAUSSIAN_COUNT) {
		val index = k * CHANNEL_COUNT + channel
		data[index] += offset
		result += data[index] * weights[index]
	}
	return result
}

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

class VoiceActivityDetector(aggressiveness: Aggressiveness = Aggressiveness.Quality) {
	private var frameCount: Int = 0

	// PDF parameters
	private val noiseMeans = NOISE_DATA_MEANS.clone()
	private val speechMeans = SPEECH_DATA_MEANS.clone()
	private val noiseStds = NOISE_DATA_STDS.clone()
	private val speechStds = SPEECH_DATA_STDS.clone()

	private val ageVector = IntArray(16 * CHANNEL_COUNT) { 0 }
	private val minimumValueVector = IntArray(16 * CHANNEL_COUNT) { 10000 }

	// Splitting filter states
	private val upperState = List(5) { MutableInt(0) }
	private val lowerState = List(5) { MutableInt(0) }

	// High pass filter states
	private val highPassFilterState = IntArray(4) { 0 }

	// Median value memory for findMinimum()
	private val median = IntArray(CHANNEL_COUNT) { 1600 }

	private data class ThresholdsRecord(
		val localThreshold: Int,
		val globalThreshold: Int
	)

	private val thresholds = when(aggressiveness) {
		Aggressiveness.Quality -> ThresholdsRecord(24, 57)
		Aggressiveness.LowBitrate -> ThresholdsRecord(37, 100)
		Aggressiveness.Aggressive -> ThresholdsRecord(82, 285)
		Aggressiveness.VeryAggressive -> ThresholdsRecord(94, 1100)
	}

	/**
	 * Calculates the probabilities for both speech and background noise using Gaussian Mixture
	 * Models (GMM). A hypothesis-test is performed to decide which type of signal is most probable.
	 *
	 * @param[features] Feature vector of length [CHANNEL_COUNT] = log10(energy in frequency band)
	 * @param[totalEnergy] Total energy in audio frame.
	 * @param[frameLength] Number of input samples.
	 * @return VAD decision. True if frame contains speech, false otherwise.
	 */
	private fun getGmmProbability(features: List<Int>, totalEnergy: Int, frameLength: Int): Boolean {
		var speech = false
		var tmp: Int
		var tmp1: Int
		var tmp2: Int
		val deltaN = IntArray(TABLE_SIZE)
		val deltaS = IntArray(TABLE_SIZE)
		val ngprvec = IntArray(TABLE_SIZE) { 0 } // Conditional probability = 0.
		val sgprvec = IntArray(TABLE_SIZE) { 0 } // Conditional probability = 0.
		var sumLogLikelihoodRatios = 0
		val noiseProbability = IntArray(GAUSSIAN_COUNT)
		val speechProbability = IntArray(GAUSSIAN_COUNT)

		assert(frameLength == 80)

		if (totalEnergy > MIN_ENERGY) {
			// The signal power of current frame is large enough for processing. The processing
			// consists of two parts:
			// 1) Calculating the likelihood of speech and thereby a VAD decision.
			// 2) Updating the underlying model, w.r.t., the decision made.

			// The detection scheme is an LRT with hypothesis
			// H0: Noise
			// H1: Speech
			//
			// We combine a global LRT with local tests, for each frequency sub-band, here named
			// channel.
			for (channel in 0 until CHANNEL_COUNT) {
				// For each channel we model the probability with a GMM consisting of
				// GAUSSIAN_COUNT, with different means and standard deviations depending
				// on H0 or H1.
				var h0Test = 0
				var h1Test = 0
				for (k in 0 until GAUSSIAN_COUNT) {
					val gaussian = channel + k * CHANNEL_COUNT

					// Probability under H0, that is, probability of frame being noise.
					// Value given in Q27 = Q7 * Q20.
					val pNoise = getGaussianProbability(features[channel], noiseMeans[gaussian], noiseStds[gaussian])
					deltaN[gaussian] = pNoise.delta
					noiseProbability[k] = NOISE_DATA_WEIGHTS[gaussian] * pNoise.probability
					h0Test += noiseProbability[k] // Q27

					// Probability under H1, that is, probability of frame being speech.
					// Value given in Q27 = Q7 * Q20.
					val pSpeech = getGaussianProbability(features[channel], speechMeans[gaussian], speechStds[gaussian])
					speechProbability[k] = SPEECH_DATA_WEIGHTS[gaussian] * pSpeech.probability
					deltaS[gaussian] = pSpeech.delta
					h1Test += speechProbability[k] // Q27
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
				val shiftsH0 = if (h0Test != 0) normSigned(h0Test) else 31
				val shiftsH1 = if (h1Test != 0) normSigned(h1Test) else 31
				val logLikelihoodRatio = shiftsH0 - shiftsH1

				// Update sumLogLikelihoodRatios with spectrum weighting.
				// This is used for the global VAD decision.
				sumLogLikelihoodRatios += logLikelihoodRatio * SPECTRUM_WEIGHT[channel]

				// Local VAD decision.
				if ((logLikelihoodRatio * 4) > thresholds.localThreshold) {
					speech = true
				}

				// Calculate local noise probabilities used later when updating the GMM.
				val h0 = h0Test shr 12 // Q15
				if (h0 > 0) {
					// High probability of noise. Assign conditional probabilities for each
					// Gaussian in the GMM.
					val tmp3 = (noiseProbability[0] and 0xFFFFF000u.toInt()) shl 2 // Q29
					ngprvec[channel] = tmp3 safeDiv h0 // Q14
					ngprvec[channel + CHANNEL_COUNT] = 16384 - ngprvec[channel]
				} else {
					// Low noise probability. Assign conditional probability 1 to the first
					// Gaussian and 0 to the rest (which is already set at initialization).
					ngprvec[channel] = 16384
				}

				// Calculate local speech probabilities used later when updating the GMM.
				val h1 = h1Test shr 12 // Q15
				if (h1 > 0) {
					// High probability of speech. Assign conditional probabilities for each
					// Gaussian in the GMM. Otherwise use the initialized values, i.e., 0.
					val tmp3 = (speechProbability[0] and 0xFFFFF000u.toInt()) shl 2 // Q29
					sgprvec[channel] = tmp3 safeDiv h1 // Q14
					sgprvec[channel + CHANNEL_COUNT] = 16384 - sgprvec[channel]
				}
			}

			// Make a global VAD decision.
			speech = speech || (sumLogLikelihoodRatios >= thresholds.globalThreshold)

			// Update the model parameters.
			var maxspe = 12800
			for (channel in 0 until CHANNEL_COUNT) {
				// Get minimum value in past which is used for long term correction in Q4.
				val featureMinimum = findMinimum(features[channel], channel)

				// Compute the "global" mean, that is the sum of the two means weighted.
				var noiseGlobalMean = getWeightedAverage(noiseMeans, channel, 0, NOISE_DATA_WEIGHTS)
				tmp1 = noiseGlobalMean shr 6 // Q8

				for (k in 0 until GAUSSIAN_COUNT) {
					val gaussian = channel + k * CHANNEL_COUNT

					val nmk = noiseMeans[gaussian]
					val smk = speechMeans[gaussian]
					var nsk = noiseStds[gaussian]
					var ssk = speechStds[gaussian]

					// Update noise mean vector if the frame consists of noise only.
					var nmk2 = nmk
					if (!speech) {
						// deltaN = (x-mu)/sigma^2
						// ngprvec[k] = noiseProbability[k] / (noiseProbability[0] + noiseProbability[1])

						// (Q14 * Q11 shr 11) = Q14.
						val delt = (ngprvec[gaussian] * deltaN[gaussian]) shr 11
						// Q7 + (Q14 * Q15 shr 22) = Q7.
						nmk2 = nmk + ((delt * NOISE_UPDATE_CONST) shr 22)
					}

					// Long term correction of the noise mean.
					// Q8 - Q8 = Q8.
					val ndelt = (featureMinimum shl 4) - tmp1
					// Q7 + (Q8 * Q8) shr 9 = Q7.
					var nmk3 = nmk2 + ((ndelt * BACK_ETA) shr 9)

					// Control that the noise mean does not drift to much.
					tmp = (k + 5) shl 7
					if (nmk3 < tmp) {
						nmk3 = tmp
					}
					tmp = (72 + k - channel) shl 7
					if (nmk3 > tmp) {
						nmk3 = tmp
					}
					noiseMeans[gaussian] = nmk3

					if (speech) {
						// Update speech mean vector:
						// deltaS = (x-mu)/sigma^2
						// sgprvec[k] = speechProbability[k] / (speechProbability[0] + speechProbability[1])

						// (Q14 * Q11) shr 11 = Q14.
						val delt = (sgprvec[gaussian] * deltaS[gaussian]) shr 11
						// Q14 * Q15 shr 21 = Q8.
						tmp = (delt * SPEECH_UPDATE_CONST) shr 21
						// Q7 + (Q8 shr 1) = Q7. With rounding.
						var smk2 = smk + ((tmp + 1) shr 1)

						// Control that the speech mean does not drift to much.
						val maxmu = maxspe + 640
						if (smk2 < MINIMUM_MEAN[k]) {
							smk2 = MINIMUM_MEAN[k]
						}
						if (smk2 > maxmu) {
							smk2 = maxmu
						}
						speechMeans[gaussian] = smk2 // Q7.

						// (Q7 shr 3) = Q4. With rounding.
						tmp = (smk + 4) shr 3

						tmp = features[channel] - tmp // Q4
						// (Q11 * Q4 shr 3) = Q12.
						var tmp4 = (deltaS[gaussian] * tmp) shr 3
						var tmp5 = tmp4 - 4096
						tmp = sgprvec[gaussian] shr 2
						// (Q14 shr 2) * Q12 = Q24.
						tmp4 = tmp * tmp5

						tmp5 = tmp4 shr 4 // Q20

						// 0.1 * Q20 / Q7 = Q13.
						if (tmp5 > 0) {
							tmp = tmp5 safeDiv (ssk * 10)
						} else {
							tmp = -tmp5 safeDiv (ssk * 10)
							tmp = -tmp
						}
						// Divide by 4 giving an update factor of 0.025 (= 0.1 / 4).
						// Note that division by 4 equals shift by 2, hence,
						// (Q13 shr 8) = (Q13 shr 6) / 4 = Q7.
						tmp += 128 // Rounding.
						ssk += (tmp shr 8)
						if (ssk < MIN_STD) {
							ssk = MIN_STD
						}
						speechStds[gaussian] = ssk
					} else {
						// Update GMM variance vectors.
						// deltaN * (features[channel] - nmk) - 1
						// Q4 - (Q7 shr 3) = Q4.
						tmp = features[channel] - (nmk shr 3)
						// (Q11 * Q4 shr 3) = Q12.
						var tmp5 = (deltaN[gaussian] * tmp) shr 3
						tmp5 -= 4096

						// (Q14 shr 2) * Q12 = Q24.
						tmp = (ngprvec[gaussian] + 2) shr 2
						val tmp4 = tmp * tmp5
						// Q20  * approx 0.001 (2^-10=0.0009766), hence,
						// (Q24 shr 14) = (Q24 shr 4) / 2^10 = Q20.
						tmp5 = tmp4 shr 14

						// Q20 / Q7 = Q13.
						if (tmp5 > 0) {
							tmp = tmp5 safeDiv nsk
						} else {
							tmp = -tmp5 safeDiv nsk
							tmp = -tmp
						}
						tmp += 32 // Rounding
						nsk += tmp shr 6 // Q13 shr 6 = Q7.
						if (nsk < MIN_STD) {
							nsk = MIN_STD
						}
						noiseStds[gaussian] = nsk
					}
				}

				// Separate models if they are too close.
				// noiseGlobalMean in Q14 (= Q7 * Q7).
				noiseGlobalMean = getWeightedAverage(noiseMeans, channel, 0, NOISE_DATA_WEIGHTS)

				// speechGlobalMean in Q14 (= Q7 * Q7).
				var speechGlobalMean = getWeightedAverage(speechMeans, channel, 0, SPEECH_DATA_WEIGHTS)

				// diff = "global" speech mean - "global" noise mean.
				// (Q14 shr 9) - (Q14 shr 9) = Q5.
				val diff = (speechGlobalMean shr 9) - (noiseGlobalMean shr 9)
				if (diff < MINIMUM_DIFFERENCE[channel]) {
					tmp = MINIMUM_DIFFERENCE[channel] - diff

					// tmp1_s16 = ~0.8 * (MINIMUM_DIFFERENCE - diff) in Q7.
					// tmp2_s16 = ~0.2 * (MINIMUM_DIFFERENCE - diff) in Q7.
					tmp1 = (13 * tmp) shr 2
					tmp2 = (3 * tmp) shr 2

					// Move Gaussian means for speech model by tmp1 and update speechGlobalMean.
					// Note that speechMeans[channel] is changed after the call.
					speechGlobalMean = getWeightedAverage(speechMeans, channel, tmp1, SPEECH_DATA_WEIGHTS)

					// Move Gaussian means for noise model by -tmp2 and update noiseGlobalMean.
					// Note that noiseMeans[channel] is
					// changed after the call.
					noiseGlobalMean = getWeightedAverage(noiseMeans, channel, -tmp2, NOISE_DATA_WEIGHTS)
				}

				// Control that the speech & noise means do not drift to much.
				maxspe = MAXIMUM_SPEECH[channel]
				tmp2 = speechGlobalMean shr 7
				if (tmp2 > maxspe) {
					// Upper limit of speech model.
					tmp2 -= maxspe

					for (k in 0 until GAUSSIAN_COUNT) {
						speechMeans[channel + k * CHANNEL_COUNT] -= tmp2
					}
				}

				tmp2 = noiseGlobalMean shr 7
				if (tmp2 > MAXIMUM_NOISE[channel]) {
					tmp2 -= MAXIMUM_NOISE[channel]

					for (k in 0 until GAUSSIAN_COUNT) {
						noiseMeans[channel + k * CHANNEL_COUNT] -= tmp2
					}
				}
			}
			frameCount++
		}

		return speech
	}

	/**
	 * Updates and returns the smoothed feature minimum. As minimum we use the median of the five
	 * smallest feature values in a 100 frames long window.
	 *
	 * Inserts [featureValue] into [minimumValueVector], if it is one of the 16 smallest values the
	 * last 100 frames. Then calculates and returns the median of the five smallest values.
	 *
	 * As long as [frameCount] is zero, that is, we haven't received any "valid" data, [findMinimum]
	 * outputs the default value of 1600.
	 *
	 * @param[featureValue] New feature value to update with.
	 * @param[channel] Channel number.
	 * @return Smoothed minimum value for a moving window.
	 */
	private fun findMinimum(featureValue: Int, channel: Int): Int {
		var position = -1
		var currentMedian = 1600
		var alpha = 0
		var tmp: Int
		val offset = channel * 16

		// Accessor for the age of each value of the channel
		val age = object {
			operator fun get(i: Int) = ageVector[offset + i]
			operator fun set(i: Int, value: Int) { ageVector[offset + i] = value }
		}

		// Accessor for the 16 minimum values of the channel
		val smallestValues = object {
			operator fun get(i: Int) = minimumValueVector[offset + i]
			operator fun set(i: Int, value: Int) { minimumValueVector[offset + i] = value }
		}

		assert(channel < CHANNEL_COUNT)

		// Each value in smallestValues is getting 1 loop older. Update age and remove old values.
		for (i in 0 until 16) {
			if (age[i] != 100) {
				age[i]++
			} else {
				// Too old value. Remove from memory and shift larger values downwards.
				for (j in i until 15) {
					smallestValues[j] = smallestValues[j + 1]
					age[j] = age[j + 1]
				}
				age[15] = 101
				smallestValues[15] = 10000
			}
		}

		// Check if featureValue is smaller than any of the values in smallest_values.
		// If so, find the position where to insert the new value.
		if (featureValue < smallestValues[7]) {
			position = when {
				featureValue < smallestValues[3] ->
					when {
						featureValue < smallestValues[1] -> if (featureValue < smallestValues[0]) 0 else 1
						featureValue < smallestValues[2] -> 2
						else -> 3
					}
				featureValue < smallestValues[5] -> if (featureValue < smallestValues[4]) 4 else 5
				featureValue < smallestValues[6] -> 6
				else -> 7
			}
		} else if (featureValue < smallestValues[15]) {
			position = when {
				featureValue < smallestValues[11] -> when {
					featureValue < smallestValues[9] -> if (featureValue < smallestValues[8]) 8 else 9
					featureValue < smallestValues[10] -> 10
					else -> 11
				}
				featureValue < smallestValues[13] -> if (featureValue < smallestValues[12]) 12 else 13
				featureValue < smallestValues[14] -> 14
				else -> 15
			}
		}

		// If we have detected a new small value, insert it at the correct position and shift larger
		// values up.
		if (position > -1) {
			for (i in 15 downTo position + 1) {
				smallestValues[i] = smallestValues[i - 1]
				age[i] = age[i - 1]
			}
			smallestValues[position] = featureValue
			age[position] = 1
		}

		// Get currentMedian
		if (frameCount > 2) {
			currentMedian = smallestValues[2]
		} else if (frameCount > 0) {
			currentMedian = smallestValues[0]
		}

		// Smooth the median value.
		if (frameCount > 0) {
			alpha = if (currentMedian < median[channel]) {
				SMOOTHING_DOWN // 0.2 in Q15.
			} else {
				SMOOTHING_UP // 0.99 in Q15.
			}
		}
		tmp = (alpha + 1) * median[channel]
		tmp += (Short.MAX_VALUE - alpha) * currentMedian
		tmp += 16384
		median[channel] = tmp shr 15

		return median[channel]
	}

	private data class FeatureResult(
		// 10 * log10(energy in each frequency band), Q4
		val features: List<Int>,
		// Total energy of the signal
		// NOTE: This value is not exact. It is only used in a comparison.
		val totalEnergy: Int
	)

	/**
	 * Takes an audio buffer and calculates the logarithm of the energy of each of the
	 * [CHANNEL_COUNT] = 6 frequency bands used by the VAD:
	 * 80-250 Hz, 250-500 Hz, 500-1000 Hz, 1000-2000 Hz, 2000-3000 Hz, 3000-4000 Hz.
	 *
	 * The values are given in Q4 and written to features. Further, an approximate overall energy is
	 * returned. The return value is used in [getGmmProbability] as a signal indicator, hence it is
	 * arbitrary above the threshold [MIN_ENERGY].
	*/
	private fun calculateFeatures(input: AudioBuffer): FeatureResult {
		assert(input.size == 80)

		// Split at 2000 Hz and downsample.
		var frequencyBand = 0
		val `0 to 4000 Hz` = input
		val (`2000 to 4000 Hz`, `0 to 2000 Hz`) =
			splitFilter(`0 to 4000 Hz`, upperState[frequencyBand], lowerState[frequencyBand])

		// For the upper band (2000 to 4000 Hz) split at 3000 Hz and downsample.
		frequencyBand = 1
		val (`3000 to 4000 Hz`, `2000 to 3000 Hz`) =
			splitFilter(`2000 to 4000 Hz`, upperState[frequencyBand], lowerState[frequencyBand])

		// For the lower band (0 to 2000 Hz) split at 1000 Hz and downsample.
		frequencyBand = 2
		val (`1000 to 2000 Hz`, `0 to 1000 Hz`) =
			splitFilter(`0 to 2000 Hz`, upperState[frequencyBand], lowerState[frequencyBand])

		// For the lower band (0 to 1000 Hz) split at 500 Hz and downsample.
		frequencyBand = 3
		val (`500 to 1000 Hz`, `0 to 500 Hz`) =
			splitFilter(`0 to 1000 Hz`, upperState[frequencyBand], lowerState[frequencyBand])

		// For the lower band (0 t0 500 Hz) split at 250 Hz and downsample.
		frequencyBand = 4
		val (`250 to 500 Hz`, `0 to 250 Hz`) =
			splitFilter(`0 to 500 Hz`, upperState[frequencyBand], lowerState[frequencyBand])

		// Remove 0 to 80 Hz by high pass filtering the lower band.
		val `80 to 250 Hz` = highPassFilter(`0 to 250 Hz`, highPassFilterState)

		val totalEnergy = MutableInt(0)
		val `energy in 3000 to 4000 Hz` = logOfEnergy(`3000 to 4000 Hz`, SPLIT_FILTER_OFFSETS[5], totalEnergy)
		val `energy in 2000 to 3000 Hz` = logOfEnergy(`2000 to 3000 Hz`, SPLIT_FILTER_OFFSETS[4], totalEnergy)
		val `energy in 1000 to 2000 Hz` = logOfEnergy(`1000 to 2000 Hz`, SPLIT_FILTER_OFFSETS[3], totalEnergy)
		val `energy in 500 to 1000 Hz` = logOfEnergy(`500 to 1000 Hz`, SPLIT_FILTER_OFFSETS[2], totalEnergy)
		val `energy in 250 to 500 Hz` = logOfEnergy(`250 to 500 Hz`, SPLIT_FILTER_OFFSETS[1], totalEnergy)
		val `energy in 50 to 250 Hz` = logOfEnergy(`80 to 250 Hz`, SPLIT_FILTER_OFFSETS[0], totalEnergy)

		val features = listOf(
			`energy in 50 to 250 Hz`,
			`energy in 250 to 500 Hz`,
			`energy in 500 to 1000 Hz`,
			`energy in 1000 to 2000 Hz`,
			`energy in 2000 to 3000 Hz`,
			`energy in 3000 to 4000 Hz`
		)
		assert(features.size == CHANNEL_COUNT)
		return FeatureResult(features, totalEnergy.toInt())
	}

	/**
	 * Calculates a VAD decision for the specified audio frame, which must be 100 ms long and
	 * sampled at 8 kHz.
	 *
	 * @return If true, the frame contains voice activity.
	 */
	fun process(audioFrame: AudioBuffer): Boolean {
		// Get power in the bands
		val (features, totalEnergy) = calculateFeatures(audioFrame)

		// Make a VAD
		val speech = getGmmProbability(features, totalEnergy, audioFrame.size)

		return speech
	}
}
