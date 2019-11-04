import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
	kotlin("jvm") version "1.3.50"
}

repositories {
	mavenCentral()
	jcenter()
}

dependencies {
	implementation(kotlin("stdlib-jdk8"))
	implementation("org.apache.commons:commons-lang3:3.9")
	implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.3.2")

	// Unit testing
	val spekVersion = "2.0.8"
	testImplementation("org.spekframework.spek2:spek-dsl-jvm:$spekVersion")
	testRuntimeOnly("org.spekframework.spek2:spek-runner-junit5:$spekVersion")
	testCompile("org.assertj:assertj-core:3.11.1")
}

val distsDirName = convention.getPlugin(BasePluginConvention::class).distsDirName
var distDirectory = File(project.buildDir, distsDirName)

tasks {
	withType<KotlinCompile> {
		kotlinOptions.jvmTarget = "1.8"
		// kotlinOptions.allWarningsAsErrors = true
		kotlinOptions.freeCompilerArgs += "-Xuse-experimental=kotlin.ExperimentalUnsignedTypes"
		kotlinOptions.freeCompilerArgs += "-Xinline-classes"
	}

	test {
		useJUnitPlatform {
			includeEngines("spek2")
		}
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
