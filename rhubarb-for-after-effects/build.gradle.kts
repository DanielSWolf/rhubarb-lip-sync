plugins {
	base
}

val distsDirName = convention.getPlugin(BasePluginConvention::class).distsDirName
var distDirectory = File(project.buildDir, distsDirName)

tasks {
	val copy by creating(Copy::class) {
		from(listOf("Rhubarb Lip Sync.jsx", "README.adoc"))
		into(distDirectory)
	}
	
	assemble {
		dependsOn(copy)
	}
}
