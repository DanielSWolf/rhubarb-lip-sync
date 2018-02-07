package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.binding.BooleanBinding
import javafx.beans.property.SimpleBooleanProperty
import javafx.beans.property.SimpleListProperty
import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import javafx.collections.ObservableList
import java.nio.file.Path
import tornadofx.getValue
import tornadofx.observable
import tornadofx.setValue
import java.util.concurrent.ExecutorService

class AnimationFileModel(animationFilePath: Path, private val executor: ExecutorService) {
	val spineJson = SpineJson(animationFilePath)

	val slotsProperty = SimpleObjectProperty<ObservableList<String>>()
	var slots by slotsProperty
		private set

	val mouthSlotProperty: SimpleStringProperty = SimpleStringProperty().alsoListen {
		mouthNaming = if (mouthSlot != null)
			MouthNaming.guess(spineJson.getSlotAttachmentNames(mouthSlot))
		else null

		mouthShapes = if (mouthSlot != null) {
			val mouthNames = spineJson.getSlotAttachmentNames(mouthSlot)
			MouthShape.values().filter { mouthNames.contains(mouthNaming.getName(it)) }
		} else listOf()

		mouthSlotError = if (mouthSlot != null) null
		else "No slot with mouth drawings specified."
	}
	var mouthSlot by mouthSlotProperty

	val mouthSlotErrorProperty = SimpleStringProperty()
	var mouthSlotError by mouthSlotErrorProperty
		private set

	val mouthNamingProperty = SimpleObjectProperty<MouthNaming>()
	var mouthNaming by mouthNamingProperty
		private set

	val mouthShapesProperty = SimpleObjectProperty<List<MouthShape>>().alsoListen {
		mouthShapesError = getMouthShapesErrorString()
	}
	var mouthShapes by mouthShapesProperty
		private set

	val mouthShapesErrorProperty = SimpleStringProperty()
	var mouthShapesError by mouthShapesErrorProperty
		private set

	val audioFileModelsProperty = SimpleListProperty<AudioFileModel>(
		spineJson.audioEvents
			.map { event ->
				AudioFileModel(event, this, executor, { result -> saveAnimation(result, event.name) })
			}
			.observable()
	)
	val audioFileModels by audioFileModelsProperty

	val busyProperty = SimpleBooleanProperty().apply {
		bind(object : BooleanBinding() {
			init {
				for (audioFileModel in audioFileModels) {
					super.bind(audioFileModel.busyProperty)
				}
			}
			override fun computeValue(): Boolean {
				return audioFileModels.any { it.busy }
			}
		})
	}
	val busy by busyProperty

	val validProperty = SimpleBooleanProperty().apply {
		val errorProperties = arrayOf(mouthSlotErrorProperty, mouthShapesErrorProperty)
		bind(object : BooleanBinding() {
			init {
				super.bind(*errorProperties)
			}
			override fun computeValue(): Boolean {
				return errorProperties.all { it.value == null }
			}
		})
	}
	val valid by validProperty

	private fun saveAnimation(mouthCues: List<MouthCue>, audioEventName: String) {
		val animationName = getAnimationName(audioEventName)
		spineJson.createOrUpdateAnimation(mouthCues, audioEventName, animationName, mouthSlot, mouthNaming)
		spineJson.save()
	}

	private fun getAnimationName(audioEventName: String): String = "say_$audioEventName"

	init {
		slots = spineJson.slots.observable()
		mouthSlot = spineJson.guessMouthSlot()
	}

	private fun getMouthShapesErrorString(): String? {
		val missingBasicShapes = MouthShape.basicShapes
			.filter{ !mouthShapes.contains(it) }
		if (missingBasicShapes.isEmpty()) return null

		val result = StringBuilder()
		val missingShapesString = missingBasicShapes.joinToString()
		result.appendln(
			if (missingBasicShapes.count() > 1)
				"Mouth shapes $missingShapesString are missing."
			else
				"Mouth shape $missingShapesString is missing."
		)

		val first = MouthShape.basicShapes.first()
		val last = MouthShape.basicShapes.last()
		result.append("At least the basic mouth shapes $first-$last need corresponding image attachments.")
		return result.toString()
	}

}