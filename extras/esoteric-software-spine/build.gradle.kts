import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import java.io.File

plugins {
	kotlin("jvm") version "1.6.0"
	id("org.openjfx.javafxplugin") version "0.0.10"
}

fun getVersion(): String {
	// Dynamically read version from CMake file
	val file = File(rootDir.parentFile.parentFile, "app-info.toml")
	val text = file.readText()
	return Regex("""appVersion\s*=\s*"(.*?)"(?:)""").find(text)!!.groupValues[1]
}

group = "com.rhubarb_lip_sync"
version = getVersion()

repositories {
	mavenCentral()
	jcenter()
	maven("https://oss.sonatype.org/content/repositories/snapshots")
}

dependencies {
	implementation("org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.6.0")
	implementation("com.beust:klaxon:5.5")
	implementation("org.apache.commons:commons-lang3:3.12.0")
	implementation("no.tornado:tornadofx:2.0.0-SNAPSHOT")
	testImplementation("org.junit.jupiter:junit-jupiter:5.8.1")
	testImplementation("org.assertj:assertj-core:3.21.0")
}

javafx {
    version = "15.0.1"
    modules("javafx.controls")
}

tasks.withType<KotlinCompile> {
	kotlinOptions.jvmTarget = "1.8"
}

tasks.test {
	useJUnitPlatform()
}

tasks.withType<Jar> {
	manifest {
		attributes("Main-Class" to "com.rhubarb_lip_sync.rhubarb_for_spine.MainKt")
	}

	from(configurations.compileClasspath.get().map { if (it.isDirectory) it else zipTree(it) })
}
