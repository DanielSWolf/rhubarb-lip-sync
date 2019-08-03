tasks {
	var distDirectory = File(project.buildDir, "distributions")

	val assemble by creating(Copy::class) {
		from(listOf("Rhubarb Lip Sync.jsx", "README.adoc"))
		into(distDirectory)
	}

	val build by creating {
		dependsOn(assemble)
	}
}
