package com.rhubarb_lip_sync.rhubarb_for_spine

// Modeled after C#'s IProgress<double>
interface Progress {
	fun reportProgress(progress: Double)
}