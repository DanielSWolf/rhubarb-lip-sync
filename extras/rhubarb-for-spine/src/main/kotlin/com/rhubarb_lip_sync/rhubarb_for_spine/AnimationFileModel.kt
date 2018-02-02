package com.rhubarb_lip_sync.rhubarb_for_spine

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
		} else null

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

	private fun saveAnimation(mouthCues: List<MouthCue>, audioEventName: String) {
		val animationName = getAnimationName(audioEventName)
		spineJson.createOrUpdateAnimation(mouthCues, audioEventName, animationName, mouthSlot, mouthNaming)
		spineJson.save()
	}

	private fun getAnimationName(audioEventName: String): String = "say_$audioEventName"

	val audioFileModels by audioFileModelsProperty

	init {
		slots = spineJson.slots.observable()
		mouthSlot = spineJson.guessMouthSlot()
	}

	private fun getMouthShapesErrorString(): String? {
		val missingBasicShapes = MouthShape.basicShapes
			.filter{ !mouthShapes.contains(it) }
		if (missingBasicShapes.isEmpty()) return null

		val result = StringBuilder()
		result.append("Mouth shapes ${missingBasicShapes.joinToString()}")
		result.appendln(if (missingBasicShapes.count() > 1) " are missing." else " is missing.")

		val first = MouthShape.basicShapes.first()
		val last = MouthShape.basicShapes.last()
		result.append("At least the basic mouth shapes $first-$last need corresponding image attachments.")
		return result.toString()
	}

}