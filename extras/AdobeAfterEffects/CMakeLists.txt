cmake_minimum_required(VERSION 3.2)

set(afterEffectsFiles
	"Rhubarb Lip Sync.jsx"
	"README.adoc"
)

install(
	FILES ${afterEffectsFiles}
	DESTINATION "extras/AdobeAfterEffects"
)
