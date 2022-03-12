package com.rhubarb_lip_sync.rhubarb_for_spine

import com.beust.klaxon.*
import javafx.collections.FXCollections.observableSet
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path

class SpineJson(private val filePath: Path) {
	private val fileDirectoryPath: Path = filePath.parent
	private val json: JsonObject
	private val skeleton: JsonObject

	init {
		if (!Files.exists(filePath)) {
			throw EndUserException("File '$filePath' does not exist.")
		}
		try {
			json = Parser.default().parse(filePath.toString()) as JsonObject
		} catch (e: Exception) {
			throw EndUserException("Wrong file format. This is not a valid JSON file.")
		}
		skeleton = json.obj("skeleton") ?: throw EndUserException("JSON file is corrupted.")

		validateProperties()
	}

	private fun validateProperties() {
		imagesDirectoryPath
		audioDirectoryPath
	}

	private val imagesDirectoryPath: Path get() {
		val relativeImagesDirectory = skeleton.string("images")
			?: throw EndUserException("JSON file is incomplete: Images path is missing."
				+ " Make sure to check 'Nonessential data' when exporting.")

		val imagesDirectoryPath = fileDirectoryPath.resolve(relativeImagesDirectory).normalize()
		if (!Files.exists(imagesDirectoryPath)) {
			throw EndUserException("Could not find images directory relative to the JSON file."
				+ " Make sure the JSON file is in the same directory as the original Spine file.")
		}

		return imagesDirectoryPath
	}

	val audioDirectoryPath: Path get() {
		val relativeAudioDirectory = skeleton.string("audio")
			?: throw EndUserException("JSON file is incomplete: Audio path is missing."
			+ " Make sure to check 'Nonessential data' when exporting.")

		val audioDirectoryPath = fileDirectoryPath.resolve(relativeAudioDirectory).normalize()
		if (!Files.exists(audioDirectoryPath)) {
			throw EndUserException("Could not find audio directory relative to the JSON file."
				+ " Make sure the JSON file is in the same directory as the original Spine file.")
		}

		return audioDirectoryPath
	}

	val frameRate: Double get() {
		return skeleton.double("fps") ?: 30.0
	}

	val slots: List<String> get() {
		val slots = json.array("slots") ?: listOf<JsonObject>()
		return slots.mapNotNull { it.string("name") }
	}

	fun guessMouthSlot(): String? {
		return slots.firstOrNull { it.contains("mouth", ignoreCase = true) }
			?: slots.firstOrNull()
	}

	data class AudioEvent(val name: String, val relativeAudioFilePath: String, val dialog: String?)

	val audioEvents: List<AudioEvent> get() {
		val events = json.obj("events") ?: JsonObject()
		val result = mutableListOf<AudioEvent>()
		for ((name, value) in events) {
			if (value !is JsonObject) throw EndUserException("Invalid event found.")

			val relativeAudioFilePath = value.string("audio") ?: continue

			val dialog = value.string("string")
			result.add(AudioEvent(name, relativeAudioFilePath, dialog))
		}
		return result
	}

	fun getSlotAttachmentNames(slotName: String): List<String> {
		@Suppress("UNCHECKED_CAST")
		val skins: Collection<JsonObject> = when (val skinsObject = json["skins"]) {
			is JsonObject -> skinsObject.values as Collection<JsonObject>
			is JsonArray<*> -> skinsObject as Collection<JsonObject>
			else -> emptyList()
		}

		// Get attachment names for all skins
		return skins
			.flatMap { skin ->
				skin.obj(slotName)?.keys?.toList()
					?: skin.obj("attachments")?.obj(slotName)?.keys?.toList()
					?: emptyList<String>()
			}
			.distinct()
	}

	val animationNames = observableSet<String>(
		json.obj("animations")?.map{ it.key }?.toMutableSet() ?: mutableSetOf()
	)

	fun createOrUpdateAnimation(mouthCues: List<MouthCue>, eventName: String, animationName: String,
		mouthSlot: String, mouthNaming: MouthNaming
	) {
		if (!json.containsKey("animations")) {
			json["animations"] = JsonObject()
		}
		val animations: JsonObject = json.obj("animations")!!

		// Round times to full frames. Always round down.
		// If events coincide, prefer the latest one.
		val keyframes = mutableMapOf<Int, MouthShape>()
		for (mouthCue in mouthCues) {
			val frameNumber = (mouthCue.time * frameRate).toInt()
			keyframes[frameNumber] = mouthCue.mouthShape
		}

		animations[animationName] = JsonObject().apply {
			this["slots"] = JsonObject().apply {
				this[mouthSlot] = JsonObject().apply {
					this["attachment"] = JsonArray(
						keyframes
							.toSortedMap()
							.map { (frameNumber, mouthShape) ->
								JsonObject().apply {
									this["time"] = frameNumber / frameRate
									this["name"] = mouthNaming.getName(mouthShape)
								}
							}
					)
				}
			}
			this["events"] = JsonArray(
				JsonObject().apply {
					this["time"] = 0.0
					this["name"] = eventName
					this["string"] = ""
				}
			)
		}

		animationNames.add(animationName)
	}

	override fun toString(): String {
		return json.toJsonString(prettyPrint = true)
	}

	fun save() {
		Files.write(filePath, listOf(toString()), StandardCharsets.UTF_8)
	}
}
