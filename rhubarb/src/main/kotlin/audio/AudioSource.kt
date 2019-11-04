package com.rhubarb_lip_sync.audio

typealias SampleArray = FloatArray

/** A sampled monaural audio signal with a fixed duration */
interface AudioSource {
	/** The sample rate in Hz */
	val sampleRate: Int

	/** The number of samples */
	val size: Int

	/**
	 * Returns a new sample array containing the specified audio segment.
	 *
	 * @param start The index of the first sample to be included
	 * @param end The index of the first sample *not* to be included
	 *
	 * @throws IllegalArgumentException if [start] < 0 or [start] > [end] or [end] > [size]
	 */
	suspend fun getSamples(start: Int, end: Int): SampleArray
}

/** An audio source that requires closing to free resources */
interface AutoCloseableAudioSource : AudioSource, AutoCloseable
