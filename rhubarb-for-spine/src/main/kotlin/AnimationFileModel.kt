package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.binding.BooleanBinding
import javafx.beans.property.SimpleBooleanProperty
import javafx.beans.property.SimpleListProperty
import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import javafx.collections.ObservableList
import tornadofx.asObservable
import java.nio.file.Path
import tornadofx.getValue
import tornadofx.observable
import tornadofx.setValue
import java.util.concurrent.ExecutorService

class AnimationFileModel(val parentModel: MainModel, animationFilePath: Path, private val executor: ExecutorService) {
	val spineJson = SpineJson(animationFilePath)

	val slotsProperty = SimpleObjectProperty<ObservableList<String>>()
	private var slots: ObservableList<String> by slotsProperty

	val mouthSlotProperty: SimpleStringProperty = SimpleStringProperty().alsoListen {
		val mouthSlot = this.mouthSlot
		val mouthNaming = if (mouthSlot != null)
			MouthNaming.guess(spineJson.getSlotAttachmentNames(mouthSlot))
		else null
		this.mouthNaming = mouthNaming

		mouthShapes = if (mouthSlot != null && mouthNaming != null) {
			val mouthNames = spineJson.getSlotAttachmentNames(mouthSlot)
			MouthShape.values().filter { mouthNames.contains(mouthNaming.getName(it)) }
		} else listOf()

		mouthSlotError = if (mouthSlot != null)
			null
		else
			"No slot with mouth drawings specified."
	}
	private var mouthSlot: String? by mouthSlotProperty

	val mouthSlotErrorProperty = SimpleStringProperty()
	private var mouthSlotError: String? by mouthSlotErrorProperty

	val mouthNamingProperty = SimpleObjectProperty<MouthNaming>()
	private var mouthNaming: MouthNaming? by mouthNamingProperty

	val mouthShapesProperty = SimpleObjectProperty<List<MouthShape>>().alsoListen {
		mouthShapesError = getMouthShapesErrorString()
	}
	var mouthShapes: List<MouthShape> by mouthShapesProperty
		private set

	val mouthShapesErrorProperty = SimpleStringProperty()
	private var mouthShapesError: String? by mouthShapesErrorProperty

	val audioFileModelsProperty = SimpleListProperty<AudioFileModel>(
		spineJson.audioEvents
			.map { event ->
				var audioFileModel: AudioFileModel? = null
				val reportResult: (List<MouthCue>) -> Unit =
					{ result -> saveAnimation(audioFileModel!!.animationName, event.name, result) }
				audioFileModel = AudioFileModel(event, this, executor, reportResult)
				return@map audioFileModel
			}
			.asObservable()
	)
	val audioFileModels: ObservableList<AudioFileModel> by audioFileModelsProperty

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

	private fun saveAnimation(animationName: String, audioEventName: String, mouthCues: List<MouthCue>) {
		spineJson.createOrUpdateAnimation(mouthCues, audioEventName, animationName, mouthSlot!!, mouthNaming!!)
		spineJson.save()
	}

	init {
		slots = spineJson.slots.asObservable()
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