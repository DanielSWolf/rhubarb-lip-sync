package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.application.Platform
import javafx.beans.property.Property

val List<String>.commonPrefix: String get() {
	return if (isEmpty()) "" else this.reduce { result, string -> result.commonPrefixWith(string) }
}

val List<String>.commonSuffix: String get() {
	return if (isEmpty()) "" else this.reduce { result, string -> result.commonSuffixWith(string) }
}

fun <TValue, TProperty : Property<TValue>> TProperty.applyListener(listener: (TValue) -> Unit) : TProperty {
	// Notify the listener of the initial value.
	// If we did this synchronously, the listener's state would have to be fully initialized the
	// moment this function is called. So calling this function during object initialization might
	// result in access to uninitialized state.
	Platform.runLater { listener(this.value) }

	addListener({ _, _, newValue -> listener(newValue)})
	return this
}
