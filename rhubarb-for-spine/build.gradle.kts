import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
	kotlin("jvm") version "1.3.41"
}

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

val distsDirName = convention.getPlugin(BasePluginConvention::class).distsDirName
var distDirectory = File(project.buildDir, distsDirName)

tasks {
	withType<KotlinCompile> {
		kotlinOptions.jvmTarget = "1.8"
	}

	test {
		useJUnitPlatform()
	}

	val copyDoc by creating(Copy::class) {
		from("README.adoc")
		into(distDirectory)
	}

	assemble {
		dependsOn(copyDoc)
	}

	jar {
		destinationDirectory.set(distDirectory)

		manifest {
			attributes("Main-Class" to "com.rhubarb_lip_sync.rhubarb_for_spine.MainKt")
		}

		from(configurations.compileClasspath.get().map { if (it.isDirectory) it else zipTree(it) })
	}
}
