package com.rhubarb_lip_sync.rhubarb_for_spine

import com.beust.klaxon.JsonObject
import com.beust.klaxon.Parser as JsonParser
import org.apache.commons.lang3.SystemUtils.IS_OS_WINDOWS
import java.io.*
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.Callable

class RhubarbTask(
	val audioFilePath: Path,
	val recognizer: String,
	val dialog: String?,
	val extendedMouthShapes: Set<MouthShape>,
	val reportProgress: (Double?) -> Unit
) : Callable<List<MouthCue>> {

	override fun call(): List<MouthCue> {
		if (Thread.currentThread().isInterrupted) {
			throw InterruptedException()
		}
		if (!Files.exists(audioFilePath)) {
			throw EndUserException("File '$audioFilePath' does not exist.")
		}

		val dialogFile = if (dialog != null) TemporaryTextFile(dialog) else null
		val outputFile = TemporaryTextFile()
		dialogFile.use { outputFile.use {
			val processBuilder = ProcessBuilder(createProcessBuilderArgs(dialogFile?.filePath)).apply {
				// See http://java-monitor.com/forum/showthread.php?t=4067
				redirectOutput(outputFile.filePath.toFile())
			}
			val process: Process = processBuilder.start()
			val stderr = BufferedReader(InputStreamReader(process.errorStream, StandardCharsets.UTF_8))
			try {
				while (true) {
					val line = stderr.interruptibleReadLine()
					val message = parseJsonObject(line)
					when (message.string("type")!!) {
						"progress" -> {
							reportProgress(message.double("value")!!)
						}
						"success" -> {
							reportProgress(1.0)
							val resultString = String(Files.readAllBytes(outputFile.filePath), StandardCharsets.UTF_8)
							return parseRhubarbResult(resultString)
						}
						"failure" -> {
							throw EndUserException(message.string("reason") ?: "Rhubarb failed without reason.")
						}
					}
				}
			} catch (e: InterruptedException) {
				process.destroyForcibly()
				throw e
			} catch (e: EOFException) {
				throw EndUserException("Rhubarb terminated unexpectedly.")
			} finally {
				process.waitFor()
			}
		}}

		throw EndUserException("Audio file processing terminated in an unexpected way.")
	}

	private fun parseRhubarbResult(jsonString: String): List<MouthCue> {
		val json = parseJsonObject(jsonString)
		val mouthCues = json.array<JsonObject>("mouthCues")!!
		return mouthCues.map { mouthCue ->
			val time = mouthCue.double("start")!!
			val mouthShape = MouthShape.valueOf(mouthCue.string("value")!!)
			return@map MouthCue(time, mouthShape)
		}
	}

	private val jsonParser = JsonParser.default()
	private fun parseJsonObject(jsonString: String): JsonObject {
		return jsonParser.parse(StringReader(jsonString)) as JsonObject
	}

	private fun createProcessBuilderArgs(dialogFilePath: Path?): List<String> {
		val extendedMouthShapesString =
			if (extendedMouthShapes.any()) extendedMouthShapes.joinToString(separator = "")
			else "\"\""
		return mutableListOf(
			rhubarbBinFilePath.toString(),
			"--machineReadable",
			"--recognizer", recognizer,
			"--exportFormat", "json",
			"--extendedShapes", extendedMouthShapesString
		).apply {
			if (dialogFilePath != null) {
				addAll(listOf(
					"--dialogFile", dialogFilePath.toString()
				))
			}
		}.apply {
			add(audioFilePath.toString())
		}
	}

	private val guiBinDirectory: Path by lazy {
		val path = urlToPath(getLocation(RhubarbTask::class.java))
		return@lazy if (Files.isDirectory(path)) path.parent else path
	}

	private val rhubarbBinFilePath: Path by lazy {
		val rhubarbBinName = if (IS_OS_WINDOWS) "rhubarb.exe" else "rhubarb"
		var currentDirectory: Path? = guiBinDirectory
		while (currentDirectory != null) {
			val candidate: Path = currentDirectory.resolve(rhubarbBinName)
			if (Files.exists(candidate)) {
				return@lazy candidate
			}
			currentDirectory = currentDirectory.parent
		}
		throw EndUserException("Could not find Rhubarb Lip Sync executable '$rhubarbBinName'."
			+ " Expected to find it in '$guiBinDirectory' or any directory above.")
	}

	private class TemporaryTextFile(text: String = "") : AutoCloseable {
		val filePath: Path = Files.createTempFile(null, null).also {
			Files.write(it, text.toByteArray(StandardCharsets.UTF_8))
		}

		override fun close() {
			Files.delete(filePath)
		}

	}

	// Same as readLine, but can be interrupted.
	// Note that this function handles linebreak characters differently from readLine.
	// It only consumes the first linebreak character before returning and swallows any leading
	// linebreak characters.
	// This behavior is much easier to implement and doesn't make any difference for our purposes.
	private fun BufferedReader.interruptibleReadLine(): String {
		val result = StringBuilder()
		while (true) {
			val char = interruptibleReadChar()
			if (char == '\r' || char == '\n') {
				if (result.isNotEmpty()) return result.toString()
			} else {
				result.append(char)
			}
		}
	}

	private fun BufferedReader.interruptibleReadChar(): Char {
		while (true) {
			if (Thread.currentThread().isInterrupted) {
				throw InterruptedException()
			}
			if (ready()) {
				val result: Int = read()
				if (result == -1) {
					throw EOFException()
				}
				return result.toChar()
			}
			Thread.yield()
		}
	}
}
