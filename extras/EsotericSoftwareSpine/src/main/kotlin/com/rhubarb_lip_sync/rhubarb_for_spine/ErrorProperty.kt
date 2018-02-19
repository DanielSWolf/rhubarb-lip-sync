package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.property.SimpleStringProperty
import javafx.beans.property.StringProperty
import javafx.beans.value.ObservableValue
import javafx.scene.Group
import javafx.scene.Node
import javafx.scene.Parent
import javafx.scene.control.Tooltip
import javafx.scene.paint.Color
import tornadofx.addChildIfPossible
import tornadofx.circle
import tornadofx.rectangle
import tornadofx.removeFromParent

fun renderErrorIndicator(): Node {
	return Group().apply {
		isManaged = false
		circle {
			radius = 7.0
			fill = Color.ORANGERED
		}
		rectangle {
			x = -1.0
			y = -5.0
			width = 2.0
			height = 7.0
			fill = Color.WHITE
		}
		rectangle {
			x = -1.0
			y = 3.0
			width = 2.0
			height = 2.0
			fill = Color.WHITE
		}
	}
}

fun Parent.errorProperty() : StringProperty {
	return properties.getOrPut("rhubarb.errorProperty", {
		val errorIndicator: Node = renderErrorIndicator()
		val tooltip = Tooltip()
		val property = SimpleStringProperty()

		fun updateTooltipVisibility() {
			if (tooltip.text.isNotEmpty() && isFocused) {
				val bounds = localToScreen(boundsInLocal)
				tooltip.show(scene.window, bounds.minX + 5, bounds.maxY + 2)
			} else {
				tooltip.hide()
			}
		}

		focusedProperty().addListener({
			_: ObservableValue<out Boolean>, _: Boolean, _: Boolean ->
			updateTooltipVisibility()
		})

		property.addListener({
			_: ObservableValue<out String?>, _: String?, newValue: String? ->

			if (newValue != null) {
				this.addChildIfPossible(errorIndicator)

				tooltip.text = newValue
				Tooltip.install(this, tooltip)
				updateTooltipVisibility()
			} else {
				errorIndicator.removeFromParent()

				tooltip.text = ""
				tooltip.hide()
				Tooltip.uninstall(this, tooltip)
				updateTooltipVisibility()
			}
		})
		return@getOrPut property
	}) as StringProperty
}