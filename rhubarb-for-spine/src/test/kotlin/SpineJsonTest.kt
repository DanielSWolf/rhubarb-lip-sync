package com.rhubarb_lip_sync.rhubarb_for_spine

import org.junit.jupiter.api.Nested
import org.junit.jupiter.api.Test
import java.nio.file.Paths
import org.assertj.core.api.Assertions.assertThat
import org.assertj.core.api.Assertions.catchThrowable

class SpineJsonTest {
	@Nested
	inner class `file format 3_7` {
		@Test
		fun `correctly reads valid file`() {
			val path = Paths.get("src/test/data/jsonFiles/matt-3.7.json").toAbsolutePath()
			val spine = SpineJson(path)

			assertThat(spine.audioDirectoryPath)
				.isEqualTo(Paths.get("src/test/data/jsonFiles/audio").toAbsolutePath())
			assertThat(spine.frameRate).isEqualTo(30.0)
			assertThat(spine.slots).containsExactly("legs", "torso", "head", "mouth")
			assertThat(spine.guessMouthSlot()).isEqualTo("mouth")
			assertThat(spine.audioEvents).containsExactly(
				SpineJson.AudioEvent("1-have-you-heard", "1-have-you-heard.wav", null),
				SpineJson.AudioEvent("2-it's-a-tool", "2-it's-a-tool.wav", null),
				SpineJson.AudioEvent("3-and-now-you-can", "3-and-now-you-can.wav", null)
			)
			assertThat(spine.getSlotAttachmentNames("mouth")).isEqualTo(('a'..'h').map{ "mouth_$it" })
			assertThat(spine.animationNames).containsExactly("shake_head", "walk")
		}

		@Test
		fun `throws on file without nonessential data`() {
			val path = Paths.get("src/test/data/jsonFiles/matt-3.7-essential.json").toAbsolutePath()
			val throwable = catchThrowable { SpineJson(path) }
			assertThat(throwable)
				.hasMessage("JSON file is incomplete: Images path is missing. Make sure to check 'Nonessential data' when exporting.")
		}
	}

	@Nested
	inner class `file format 3_8` {
		@Test
		fun `correctly reads valid file`() {
			val path = Paths.get("src/test/data/jsonFiles/matt-3.8.json").toAbsolutePath()
			val spine = SpineJson(path)

			assertThat(spine.audioDirectoryPath)
				.isEqualTo(Paths.get("src/test/data/jsonFiles/audio").toAbsolutePath())
			assertThat(spine.frameRate).isEqualTo(30.0)
			assertThat(spine.slots).containsExactly("legs", "torso", "head", "mouth")
			assertThat(spine.guessMouthSlot()).isEqualTo("mouth")
			assertThat(spine.audioEvents).containsExactly(
				SpineJson.AudioEvent("1-have-you-heard", "1-have-you-heard.wav", null),
				SpineJson.AudioEvent("2-it's-a-tool", "2-it's-a-tool.wav", null),
				SpineJson.AudioEvent("3-and-now-you-can", "3-and-now-you-can.wav", null)
			)
			assertThat(spine.getSlotAttachmentNames("mouth")).isEqualTo(('a'..'h').map{ "mouth_$it" })
			assertThat(spine.animationNames).containsExactly("shake_head", "walk")
		}

		@Test
		fun `throws on file without nonessential data`() {
			val path = Paths.get("src/test/data/jsonFiles/matt-3.8-essential.json").toAbsolutePath()
			val throwable = catchThrowable { SpineJson(path) }
			assertThat(throwable)
				.hasMessage("JSON file is incomplete: Images path is missing. Make sure to check 'Nonessential data' when exporting.")
		}
	}
}
