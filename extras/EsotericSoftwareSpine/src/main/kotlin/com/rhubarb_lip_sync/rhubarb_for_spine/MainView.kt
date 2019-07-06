package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.beans.property.Property
import javafx.beans.property.SimpleBooleanProperty
import javafx.beans.property.SimpleObjectProperty
import javafx.beans.property.SimpleStringProperty
import javafx.event.ActionEvent
import javafx.event.EventHandler
import javafx.event.EventTarget
import javafx.geometry.Pos
import javafx.scene.control.*
import javafx.scene.input.DragEvent
import javafx.scene.input.TransferMode
import javafx.scene.layout.*
import javafx.scene.paint.Color
import javafx.scene.text.Font
import javafx.scene.text.FontWeight
import javafx.scene.text.Text
import javafx.stage.FileChooser
import javafx.util.StringConverter
import tornadofx.*
import java.io.File
import java.util.concurrent.Executors

class MainView : View() {
	private val executor = Executors.newSingleThreadExecutor()
	private val mainModel = MainModel(executor)

	init {
		title = "Rhubarb Lip Sync for Spine"
	}

	override val root = form {
		var filePathTextField: TextField? = null
		var filePathButton: Button? = null

		val fileModelProperty = mainModel.animationFileModelProperty

		minWidth = 800.0
		prefWidth = 1000.0
		fieldset("Settings") {
			disableProperty().bind(fileModelProperty.select { it!!.busyProperty })
			field("Spine JSON file") {
				filePathTextField = textfield {
					textProperty().bindBidirectional(mainModel.filePathStringProperty)
					errorProperty().bind(mainModel.filePathErrorProperty)
				}
				filePathButton = button("...")
			}
			field("Mouth slot") {
				combobox<String> {
					itemsProperty().bind(fileModelProperty.select { it!!.slotsProperty })
					valueProperty().bindBidirectional(fileModelProperty.select { it!!.mouthSlotProperty })
					errorProperty().bind(fileModelProperty.select { it!!.mouthSlotErrorProperty })
				}
			}
			field("Mouth naming") {
				label {
					textProperty().bind(
						fileModelProperty
							.select { it!!.mouthNamingProperty }
							.select { SimpleStringProperty(it.displayString) }
					)
				}
			}
			field("Mouth shapes") {
				hbox {
					errorProperty().bind(fileModelProperty.select { it!!.mouthShapesErrorProperty })
					gridpane {
						hgap = 10.0
						vgap = 3.0
						row {
							label("Basic:")
							for (shape in MouthShape.basicShapes) {
								renderShapeCheckbox(shape, fileModelProperty, this)
							}
						}
						row {
							label("Extended:")
							for (shape in MouthShape.extendedShapes) {
								renderShapeCheckbox(shape, fileModelProperty, this)
							}
						}
					}
				}
			}
			field("Dialog recognizer") {
				combobox<Recognizer> {
					itemsProperty().bind(mainModel.recognizersProperty)
					this.converter = object : StringConverter<Recognizer>() {
						override fun toString(recognizer: Recognizer?): String {
							return recognizer?.description ?: ""
						}
						override fun fromString(string: String?): Recognizer {
							throw NotImplementedError()
						}
					}
					valueProperty().bindBidirectional(mainModel.recognizerProperty)
				}
			}
			field("Animation naming") {
				textfield {
					maxWidth = 100.0
					textProperty().bindBidirectional(mainModel.animationPrefixProperty)
				}
				label("<audio event name>")
				textfield {
					maxWidth = 100.0
					textProperty().bindBidirectional(mainModel.animationSuffixProperty)
				}
			}
		}
		fieldset("Audio events") {
			tableview<AudioFileModel> {
				placeholder = Label("There are no events with associated audio files.")
				columnResizePolicy = SmartResize.POLICY
				column("Event", AudioFileModel::eventNameProperty)
					.weightedWidth(1.0)
				column("Animation name", AudioFileModel::animationNameProperty)
					.weightedWidth(1.0)
				column("Audio file", AudioFileModel::displayFilePathProperty)
					.weightedWidth(1.0)
				column("Dialog", AudioFileModel::dialogProperty).apply {
					weightedWidth(3.0)
					// Make dialog column wrap
					setCellFactory { tableColumn ->
						return@setCellFactory TableCell<AudioFileModel, String>().also { cell ->
							cell.graphic = Text().apply {
								textProperty().bind(cell.itemProperty())
								fillProperty().bind(cell.textFillProperty())
								val widthProperty = tableColumn.widthProperty()
									.minus(cell.paddingLeftProperty)
									.minus(cell.paddingRightProperty)
								wrappingWidthProperty().bind(widthProperty)
							}
							cell.prefHeight = Control.USE_COMPUTED_SIZE
						}
					}
				}
				column("Status", AudioFileModel::audioFileStateProperty).apply {
					weightedWidth(1.0)
					setCellFactory {
						return@setCellFactory object : TableCell<AudioFileModel, AudioFileState>() {
							override fun updateItem(state: AudioFileState?, empty: Boolean) {
								super.updateItem(state, empty)
								graphic = if (state != null) {
									when (state.status) {
										AudioFileStatus.NotAnimated -> Text("Not animated").apply {
											fill = Color.GRAY
										}
										AudioFileStatus.Pending,
										AudioFileStatus.Animating -> HBox().apply {
											val progress: Double? = state.progress
											val indeterminate = -1.0
											val bar = progressbar(progress ?: indeterminate) {
												maxWidth = Double.MAX_VALUE
											}
											HBox.setHgrow(bar, Priority.ALWAYS)
											hbox {
												minWidth = 30.0
												if (progress != null) {
													text("${(progress * 100).toInt()}%") {
														alignment = Pos.BASELINE_RIGHT
													}
												}
											}
										}
										AudioFileStatus.Canceling -> Text("Canceling")
										AudioFileStatus.Done -> Text("Done").apply {
											font = Font.font(font.family, FontWeight.BOLD, font.size)
										}
									}
								} else null
							}
						}
					}
				}
				column("", AudioFileModel::actionLabelProperty).apply {
					weightedWidth(1.0)
					// Show button
					setCellFactory {
						return@setCellFactory object : TableCell<AudioFileModel, String>() {
							override fun updateItem(item: String?, empty: Boolean) {
								super.updateItem(item, empty)
								graphic = if (!empty)
									Button(item).apply {
										this.maxWidth = Double.MAX_VALUE
										setOnAction {
											val audioFileModel = this@tableview.items[index]
											audioFileModel.performAction()
										}
										val invalidProperty: Property<Boolean> = fileModelProperty
											.select { it!!.validProperty }
											.select { SimpleBooleanProperty(!it) }
										disableProperty().bind(invalidProperty)
									}
								else
									null
							}
						}
					}
				}
				itemsProperty().bind(fileModelProperty.select { it!!.audioFileModelsProperty })
			}
		}

		onDragOver = EventHandler<DragEvent> { event ->
			if (event.dragboard.hasFiles() && mainModel.animationFileModel?.busy != true) {
				event.acceptTransferModes(TransferMode.COPY)
				event.consume()
			}
		}
		onDragDropped = EventHandler<DragEvent> { event ->
			if (event.dragboard.hasFiles() && mainModel.animationFileModel?.busy != true) {
				filePathTextField!!.text = event.dragboard.files.firstOrNull()?.path
				event.isDropCompleted = true
				event.consume()
			}
		}

		whenUndocked {
			executor.shutdownNow()
		}

		filePathButton!!.onAction = EventHandler<ActionEvent> {
			val fileChooser = FileChooser().apply {
				title = "Open Spine JSON file"
				extensionFilters.addAll(
					FileChooser.ExtensionFilter("Spine JSON file (*.json)", "*.json"),
					FileChooser.ExtensionFilter("All files (*.*)", "*.*")
				)
				val lastDirectory = filePathTextField!!.text?.let { File(it).parentFile }
				if (lastDirectory != null && lastDirectory.isDirectory) {
					initialDirectory = lastDirectory
				}
			}
			val file = fileChooser.showOpenDialog(this@MainView.primaryStage)
			if (file != null) {
				filePathTextField!!.text = file.path
			}
		}
	}

	private fun renderShapeCheckbox(shape: MouthShape, fileModelProperty: SimpleObjectProperty<AnimationFileModel?>, parent: EventTarget) {
		parent.label {
			textProperty().bind(
				fileModelProperty
					.select { it!!.mouthShapesProperty }
					.select { mouthShapes ->
						val hairSpace = "\u200A"
						val result = shape.toString() + hairSpace + if (mouthShapes.contains(shape)) "☑" else "☐"
						return@select SimpleStringProperty(result)
					}
			)
		}
	}
}
