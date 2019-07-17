package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.application.Platform
import javafx.beans.property.Property
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock
import java.io.PrintWriter
import java.io.StringWriter

val List<String>.commonPrefix: String get() {
	return if (isEmpty()) "" else this.reduce { result, string -> result.commonPrefixWith(string) }
}

val List<String>.commonSuffix: String get() {
	return if (isEmpty()) "" else this.reduce { result, string -> result.commonSuffixWith(string) }
}

fun <TValue, TProperty : Property<TValue>> TProperty.alsoListen(listener: (TValue) -> Unit) : TProperty {
	// Notify the listener of the initial value.
	// If we did this synchronously, the listener's state would have to be fully initialized the
	// moment this function is called. So calling this function during object initialization might
	// result in access to uninitialized state.
	Platform.runLater { listener(this.value) }

	addListener({ _, _, newValue -> listener(newValue)})
	return this
}

fun getExceptionMessage(action: () -> Unit): String? {
	try {
		action()
	} catch (e: Exception) {
		return e.message
	}
	return null
}

/**
 * Invokes a Runnable on the JFX thread and waits until it's finished.
 * Similar to SwingUtilities.invokeAndWait.
 * Based on http://www.guigarage.com/2013/01/invokeandwait-for-javafx/
 *
 * @throws InterruptedException Execution was interrupted
 * @throws Throwable An exception occurred in the run method of the Runnable
 */
fun runAndWait(action: () -> Unit) {
	if (Platform.isFxApplicationThread()) {
		action()
	} else {
		val lock = ReentrantLock()
		lock.withLock {
			val doneCondition = lock.newCondition()
			var throwable: Throwable? = null
			Platform.runLater {
				lock.withLock {
					try {
						action()
					} catch (e: Throwable) {
						throwable = e
					} finally {
						doneCondition.signal()
					}
				}
			}
			doneCondition.await()
			throwable?.let { throw it }
		}
	}
}

fun getStackTrace(e: Exception): String {
	val stringWriter = StringWriter()
	e.printStackTrace(PrintWriter(stringWriter))
	return stringWriter.toString()
}