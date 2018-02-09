package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.application.Platform
import javafx.beans.binding.BooleanBinding
import javafx.beans.binding.ObjectBinding
import javafx.beans.binding.StringBinding
import javafx.beans.property.SimpleBooleanProperty
import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import tornadofx.getValue
import tornadofx.setValue
import java.util.concurrent.ExecutorService
import java.util.concurrent.Future

class AudioFileModel(
	audioEvent: SpineJson.AudioEvent,
	private val parentModel: AnimationFileModel,
	private val executor: ExecutorService,
	private val reportResult: (List<MouthCue>) -> Unit
) {
	val spineJson = parentModel.spineJson

	val audioFilePath = spineJson.audioDirectoryPath.resolve(audioEvent.relativeAudioFilePath)

	val eventNameProperty = SimpleStringProperty(audioEvent.name)
	val eventName by eventNameProperty

	val displayFilePathProperty = SimpleStringProperty(audioEvent.relativeAudioFilePath)
	val displayFilePath by displayFilePathProperty

	val animationNameProperty = SimpleStringProperty().apply {
		val mainModel = parentModel.parentModel
		bind(object : ObjectBinding<String>() {
			init {
				super.bind(
					mainModel.animationPrefixProperty,
					eventNameProperty,
					mainModel.animationSuffixProperty
				)
			}
			override fun computeValue(): String {
				return mainModel.animationPrefix + eventName + mainModel.animationSuffix
			}
		})
	}
	val animationName by animationNameProperty

	val dialogProperty = SimpleStringProperty(audioEvent.dialog)
	val dialog: String? by dialogProperty

	val animationProgressProperty = SimpleObjectProperty<Double?>(null)
	var animationProgress by animationProgressProperty
		private set

	private val animatedProperty = SimpleBooleanProperty().apply {
		bind(object : ObjectBinding<Boolean>() {
			init {
				super.bind(animationNameProperty, parentModel.spineJson.animationNames)
			}
			override fun computeValue(): Boolean {
				return parentModel.spineJson.animationNames.contains(animationName)
			}
		})
	}
	private var animated by animatedProperty

	private val futureProperty = SimpleObjectProperty<Future<*>?>()
	private var future by futureProperty

	private val audioFileStateProperty = SimpleObjectProperty<AudioFileState>().apply {
		bind(object : ObjectBinding<AudioFileState>() {
			init {
				super.bind(animatedProperty, futureProperty, animationProgressProperty)
			}
			override fun computeValue(): AudioFileState {
				return if (future != null) {
					if (animationProgress != null)
						if (future!!.isCancelled)
							AudioFileState(AudioFileStatus.Canceling)
						else
							AudioFileState(AudioFileStatus.Animating, animationProgress)
					else
						AudioFileState(AudioFileStatus.Pending)
				} else {
					if (animated)
						AudioFileState(AudioFileStatus.Done)
					else
						AudioFileState(AudioFileStatus.NotAnimated)
				}
			}
		})
	}
	private val audioFileState by audioFileStateProperty

	val busyProperty = SimpleBooleanProperty().apply {
		bind(object : BooleanBinding() {
			init {
				super.bind(futureProperty)
			}
			override fun computeValue(): Boolean {
				return future != null
			}

		})
	}
	val busy by busyProperty

	val statusLabelProperty = SimpleStringProperty().apply {
		bind(object : StringBinding() {
			init {
				super.bind(audioFileStateProperty)
			}
			override fun computeValue(): String {
				return when (audioFileState.status) {
					AudioFileStatus.NotAnimated -> "Not animated"
					AudioFileStatus.Pending -> "Waiting"
					AudioFileStatus.Animating -> "${((animationProgress ?: 0.0) * 100).toInt()}%"
					AudioFileStatus.Canceling -> "Canceling"
					AudioFileStatus.Done -> "Done"
				}
			}
		})
	}
	val statusLabel by statusLabelProperty

	val actionLabelProperty = SimpleStringProperty().apply {
		bind(object : StringBinding() {
			init {
				super.bind(futureProperty)
			}
			override fun computeValue(): String {
				return if (future != null)
					"Cancel"
				else
					"Animate"
			}
		})
	}
	val actionLabel by actionLabelProperty

	fun performAction() {
		if (future == null) {
			if (animated) {
				Alert(Alert.AlertType.CONFIRMATION).apply {
					headerText = "Animation '$animationName' already exists."
					contentText = "Do you want to replace the existing animation?"
					val result = showAndWait()
					if (result.get() != ButtonType.OK) {
						return
					}
				}
			}

			startAnimation()
		} else {
			cancelAnimation()
		}
	}

	private fun startAnimation() {
		val wrapperTask = Runnable {
			val extendedMouthShapes = parentModel.mouthShapes.filter { it.isExtended }.toSet()
			val reportProgress: (Double?) -> Unit = {
				progress -> runAndWait { this@AudioFileModel.animationProgress = progress }
			}
			val rhubarbTask = RhubarbTask(audioFilePath, dialog, extendedMouthShapes, reportProgress)
			try {
				try {
					val result = rhubarbTask.call()
					runAndWait {
						reportResult(result)
					}
				} finally {
					runAndWait {
						animationProgress = null
						future = null
					}
				}
			} catch (e: InterruptedException) {
			} catch (e: Exception) {
				Platform.runLater {
					Alert(Alert.AlertType.ERROR).apply {
						headerText = "Error performing lip sync for event '$eventName'."
						contentText = e.message
						show()
					}
				}
			}

		}
		future = executor.submit(wrapperTask)
	}

	private fun cancelAnimation() {
		future?.cancel(true)
	}
}

enum class AudioFileStatus {
	NotAnimated,
	Pending,
	Animating,
	Canceling,
	Done
}

data class AudioFileState(val status: AudioFileStatus, val progress: Double? = null)