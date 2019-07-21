import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import java.io.File

plugins {
	kotlin("jvm") version "1.3.41"
}

fun getVersion(): String {
	// Dynamically read version from CMake file
	val file = File(rootDir.parentFile.parentFile, "appInfo.cmake")
	val text = file.readText()
	val major = Regex("""appVersionMajor\s+(\d+)""").find(text)!!.groupValues[1]
	val minor = Regex("""appVersionMinor\s+(\d+)""").find(text)!!.groupValues[1]
	val patch = Regex("""appVersionPatch\s+(\d+)""").find(text)!!.groupValues[1]
	val suffix = Regex("""appVersionSuffix\s+"(.*?)"""").find(text)!!.groupValues[1]
	return "$major.$minor.$patch$suffix"
}

group = "com.rhubarb_lip_sync"
version = getVersion()

repositories {
	mavenCentral()
	jcenter()
}

dependencies {
	implementation(kotlin("stdlib-jdk8"))
	implementation("com.beust:klaxon:5.0.1")
	implementation("org.apache.commons:commons-lang3:3.9")
	implementation("no.tornado:tornadofx:1.7.19")
	testImplementation("org.junit.jupiter:junit-jupiter:5.5.0")
	testCompile("org.assertj:assertj-core:3.11.1")
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
