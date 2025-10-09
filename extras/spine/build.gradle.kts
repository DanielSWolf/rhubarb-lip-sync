import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import java.io.File
import groovy.json.JsonSlurper

plugins {
    kotlin("jvm") version "1.9.25"
    id("org.openjfx.javafxplugin") version "0.1.0"
}

/** Reads the Rhubarb version from package.json. */
fun getVersion(): String {
    val file = File(rootDir.parentFile.parentFile, "package.json")
    require(file.exists()) { "${file.absolutePath} not found." }

    val json = JsonSlurper().parse(file)
    val version = when (json) {
        is Map<*, *> -> json["version"]
        else -> error("package.json did not parse to an object; got ${json?.javaClass}.")
    }

    return when (version) {
        is String -> version
        else -> error("package.json 'version' must be a string; got ${version?.javaClass}.")
    }
}

group = "com.rhubarb_lip_sync"
version = getVersion()

repositories {
    mavenCentral()
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.9.25")
    implementation("com.beust:klaxon:5.6")
    implementation("org.apache.commons:commons-lang3:3.18.0")
    implementation("no.tornado:tornadofx:1.7.20")

    testImplementation(platform("org.junit:junit-bom:5.10.2"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher:1.10.2")
    testImplementation("org.assertj:assertj-core:3.27.4")
}

javafx {
    version = "21"
    modules("javafx.controls")
}

java {
    sourceCompatibility = JavaVersion.VERSION_11
    targetCompatibility = JavaVersion.VERSION_11
}

kotlin {
    jvmToolchain(21)
}

tasks.withType<KotlinCompile> {
    kotlinOptions.jvmTarget = "11"
}

tasks.test {
    useJUnitPlatform()
}

tasks.withType<Jar> {
    manifest {
        attributes("Main-Class" to "com.rhubarb_lip_sync.rhubarb_for_spine.MainKt")
    }
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    from(configurations.runtimeClasspath.get().map { if (it.isDirectory) it else zipTree(it) })
    exclude("module-info.class", "META-INF/versions/**", "META-INF/*.SF", "META-INF/*.DSA", "META-INF/*.RSA")
    archiveFileName.set("rhubarb-for-spine.jar")
}
