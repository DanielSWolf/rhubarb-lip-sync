val distsDirName = convention.getPlugin(BasePluginConvention::class).distsDirName
var distDirectory = File(project.buildDir, distsDirName)

plugins {
	base
}

tasks {
	val copy by creating(Copy::class) {
		from(listOf(
			"Debug Rhubarb.cs",
			"Debug Rhubarb.cs.config",
			"Import Rhubarb.cs",
			"Import Rhubarb.cs.config",
			"README.adoc"
		))
		into(distDirectory)
	}

	assemble {
		dependsOn(copy)
	}
}
