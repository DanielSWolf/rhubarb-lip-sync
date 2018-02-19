package com.rhubarb_lip_sync.rhubarb_for_spine

import javafx.scene.image.Image
import javafx.stage.Stage
import tornadofx.App
import tornadofx.addStageIcon
import java.lang.reflect.Method
import javax.swing.ImageIcon

class MainApp : App(MainView::class) {
	override fun start(stage: Stage) {
		super.start(stage)
		setIcon()
	}

	private fun setIcon() {
		// Set icon for windows
		for (iconSize in listOf(16, 32, 48, 256)) {
			addStageIcon(Image(this.javaClass.getResourceAsStream("/icon-$iconSize.png")))
		}

		// OS X requires the dock icon to be changed separately.
        // Not all JDKs contain the class com.apple.eawt.Application, so we have to use reflection.
        val classLoader = this.javaClass.classLoader
        try {
            val iconURL = this.javaClass.getResource("/icon-256.png")
            val image: java.awt.Image = ImageIcon(iconURL).image

            // The following is reflection code for the line
            // Application.getApplication().setDockIconImage(image)
            val applicationClass: Class<*> = classLoader.loadClass("com.apple.eawt.Application")
            val getApplicationMethod: Method = applicationClass.getMethod("getApplication")
            val application: Any = getApplicationMethod.invoke(null)
            val setDockIconImageMethod: Method =
                applicationClass.getMethod("setDockIconImage", java.awt.Image::class.java)
            setDockIconImageMethod.invoke(application, image);
        } catch (e: Exception) {
            // Works only on OS X
        }
	}

}
