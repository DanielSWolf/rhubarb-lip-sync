package com.rhubarb_lip_sync.rhubarb_for_spine

import java.util.*

class MouthNaming(private val prefix: String, private val suffix: String, private val mouthShapeCasing: MouthShapeCasing) {

	companion object {
		fun guess(mouthNames: List<String>): MouthNaming {
			if (mouthNames.isEmpty()) {
				return MouthNaming("", "", guessMouthShapeCasing(""))
			}

			val commonPrefix = mouthNames.commonPrefix
			val commonSuffix = mouthNames.commonSuffix
			val firstMouthName = mouthNames.first()
			if (commonPrefix.length + commonSuffix.length >= firstMouthName.length) {
				return MouthNaming(commonPrefix, "", guessMouthShapeCasing(""))
			}

			val shapeName = firstMouthName.substring(
				commonPrefix.length,
				firstMouthName.length - commonSuffix.length)
			val mouthShapeCasing = guessMouthShapeCasing(shapeName)
			return MouthNaming(commonPrefix, commonSuffix, mouthShapeCasing)
		}

		private fun guessMouthShapeCasing(shapeName: String): MouthShapeCasing {
			return if (shapeName.isBlank() || shapeName[0].isLowerCase())
				MouthShapeCasing.Lower
			else
				MouthShapeCasing.Upper
		}
	}

	fun getName(mouthShape: MouthShape): String {
		val name = if (mouthShapeCasing == MouthShapeCasing.Upper)
			mouthShape.toString()
		else
			mouthShape.toString().toLowerCase(Locale.ROOT)
		return "$prefix$name$suffix"
	}

	val displayString: String get() {
		val casing = if (mouthShapeCasing == MouthShapeCasing.Upper)
			"<UPPER-CASE SHAPE NAME>"
		else
			"<lower-case shape name>"
		return "\"$prefix$casing$suffix\""
	}
}

enum class MouthShapeCasing {
	Upper,
	Lower
}