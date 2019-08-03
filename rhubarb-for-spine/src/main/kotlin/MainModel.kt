package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import javafx.collections.FXCollections
import javafx.collections.ObservableList
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

	val filePathErrorProperty = SimpleStringProperty()
	private var filePathError: String? by filePathErrorProperty

	val animationFileModelProperty = SimpleObjectProperty<AnimationFileModel?>()
	var animationFileModel by animationFileModelProperty
		private set

	val recognizersProperty = SimpleObjectProperty<ObservableList<Recognizer>>(FXCollections.observableArrayList(
		Recognizer("pocketSphinx", "PocketSphinx (use for English recordings)"),
		Recognizer("phonetic", "Phonetic (use for non-English recordings)")
	))
	private var recognizers: ObservableList<Recognizer> by recognizersProperty

	val recognizerProperty = SimpleObjectProperty<Recognizer>(recognizers[0])
	var recognizer: Recognizer by recognizerProperty

	val animationPrefixProperty = SimpleStringProperty("say_")
	var animationPrefix: String by animationPrefixProperty

	val animationSuffixProperty = SimpleStringProperty("")
	var animationSuffix: String by animationSuffixProperty

	private fun getDefaultPathString() = FX.application.parameters.raw.firstOrNull()
}

class Recognizer(val value: String, val description: String)
