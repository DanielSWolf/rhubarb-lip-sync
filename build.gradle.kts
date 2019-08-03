plugins {
	// Build scan plugin should be listed first.
	// See https://guides.gradle.org/creating-build-scans/#enable_build_scans_on_all_builds_of_your_project
	id("com.gradle.build-scan") version "2.1"

	base
}

group = "com.rhubarb_lip_sync"
version = "2.0.0-pre-alpha"

val distsDirName = convention.getPlugin(BasePluginConvention::class).distsDirName

tasks {
	val zip by creating(Zip::class) {
		subprojects.forEach { dependsOn("${it.name}:assemble") }

		for (subproject in subprojects) {
			from(File(subproject.buildDir, distsDirName)) {
				into(if (subproject.name == "rhubarb") "" else subproject.name)
			}
		}
	}

	assemble {
		dependsOn(zip)
	}
}

buildScan {
	termsOfServiceUrl = "https://gradle.com/terms-of-service"
	termsOfServiceAgree = "yes"
}
