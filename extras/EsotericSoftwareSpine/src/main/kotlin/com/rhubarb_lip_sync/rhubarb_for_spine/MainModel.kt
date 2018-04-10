package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import tornadofx.FX
import tornadofx.getValue
import tornadofx.setValue
import java.nio.file.Files
import java.nio.file.InvalidPathException
import java.nio.file.Paths
import java.util.concurrent.ExecutorService

class MainModel(private val executor: ExecutorService) {
	val filePathStringProperty = SimpleStringProperty(getDefaultPathString()).alsoListen { value ->
		filePathError = getExceptionMessage {
			animationFileModel = null
			if (value.isNullOrBlank()) {
				throw EndUserException("No input file specified.")
			}

			val path = try {
				val trimmed = value.removeSurrounding("\"")
				Paths.get(trimmed)
			} catch (e: InvalidPathException) {
				throw EndUserException("Not a valid file path.")
			}

			if (!Files.exists(path)) {
				throw EndUserException("File does not exist.")
			}

			animationFileModel = AnimationFileModel(this, path, executor)
		}
	}
	var filePathString by filePathStringProperty

	val filePathErrorProperty = SimpleStringProperty()
	var filePathError by filePathErrorProperty
		private set

	val animationFileModelProperty = SimpleObjectProperty<AnimationFileModel?>()
	var animationFileModel by animationFileModelProperty
		private set

	val animationPrefixProperty = SimpleStringProperty("say_")
	var animationPrefix by animationPrefixProperty

	val animationSuffixProperty = SimpleStringProperty("")
	var animationSuffix by animationSuffixProperty

	private fun getDefaultPathString() = FX.application.parameters.raw.firstOrNull()
}