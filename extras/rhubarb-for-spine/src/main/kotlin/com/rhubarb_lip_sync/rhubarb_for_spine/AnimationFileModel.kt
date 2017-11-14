package com.rhubarb_lip_sync.rhubarb_for_spine

import java.nio.file.Path

class AnimationFileModel(animationFilePath: Path) {
	val spineJson: SpineJson

	init {
		spineJson = SpineJson(animationFilePath)
	}
}