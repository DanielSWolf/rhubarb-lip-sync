package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.event.EventHandler
import javafx.scene.control.TextField
import javafx.scene.input.DragEvent
import javafx.scene.input.TransferMode
import tornadofx.*
import java.time.LocalDate
import java.time.Period

class MainView : View() {

	val mainModel = MainModel()

	class Person(val id: Int, val name: String, val birthday: LocalDate) {
		val age: Int get() = Period.between(birthday, LocalDate.now()).years
	}

	private val persons = listOf(
		Person(1,"Samantha Stuart",LocalDate.of(1981,12,4)),
		Person(2,"Tom Marks",LocalDate.of(2001,1,23)),
		Person(3,"Stuart Gills",LocalDate.of(1989,5,23)),
		Person(3,"Nicole Williams",LocalDate.of(1998,8,11))
	).observable()

	init {
		title = "Rhubarb Lip Sync for Spine"
	}

	override val root = form {
		var filePathField: TextField? = null

		minWidth = 800.0
		fieldset("Settings") {
			field("Spine JSON file") {
				filePathField = textfield {
					textProperty().bindBidirectional(mainModel.filePathStringProperty)
					tooltip("Hello world")
					errorProperty().bind(mainModel.filePathErrorProperty)
				}
				button("...")
			}
			field("Mouth slot") {
				textfield()
			}
			field("Mouth naming") {
				datepicker()
			}
			field("Mouth shapes") {
				textfield()
			}
		}
		fieldset("Audio events") {
			tableview(persons) {
				column("Event", Person::id)
				column("Audio file", Person::name)
				column("Dialog", Person::birthday)
				column("Status", Person::age)
				column("", Person::age)
			}
		}

		onDragOver = EventHandler<DragEvent> { event ->
			if (event.dragboard.hasFiles()) {
				event.acceptTransferModes(TransferMode.COPY)
				event.consume()
			}
		}
		onDragDropped = EventHandler<DragEvent> { event ->
			if (event.dragboard.hasFiles()) {
				filePathField!!.text = event.dragboard.files.firstOrNull()?.path
				event.isDropCompleted = true
				event.consume()
			}
		}
	}
}