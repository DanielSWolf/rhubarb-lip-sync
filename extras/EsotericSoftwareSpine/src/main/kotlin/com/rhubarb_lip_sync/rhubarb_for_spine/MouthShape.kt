package com.rhubarb_lip_sync.rhubarb_for_spine

enum class MouthShape {
	A, B, C, D, E, F, G, H, X;

	val isBasic: Boolean
		get() = this.ordinal < basicShapeCount

	val isExtended: Boolean
		get() = !this.isBasic

	companion object {
		const val basicShapeCount = 6

		val basicShapes = MouthShape.values().take(basicShapeCount)

		val extendedShapes = MouthShape.values().drop(basicShapeCount)
	}
}
