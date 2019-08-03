tasks {
	var distDirectory = File(project.buildDir, "distributions")

	val assemble by creating(Copy::class) {
		from(listOf(
			"Debug Rhubarb.cs",
			"Debug Rhubarb.cs.config",
			"Import Rhubarb.cs",
			"Import Rhubarb.cs.config",
			"README.adoc"
		))
		into(distDirectory)
	}

	create("build") {
		dependsOn(assemble)
	}
}
