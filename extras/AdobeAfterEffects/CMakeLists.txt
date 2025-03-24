cmake_minimum_required(VERSION 3.24)

install(
	FILES
		"Rhubarb Lip Sync.jsx"
		"README.adoc"
	DESTINATION "extras/AdobeAfterEffects"
)

install(
	DIRECTORY demo
	DESTINATION "extras/AdobeAfterEffects"
)
