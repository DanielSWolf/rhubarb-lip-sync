cmake_minimum_required(VERSION 3.2)

add_custom_target(
	rhubarbForSpine ALL
	"./gradlew" "build"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	COMMENT "Building Rhubarb for Spine through Gradle."
)

install(
	DIRECTORY "build/libs/"
	DESTINATION "extras/EsotericSoftwareSpine"
)

install(
	FILES README.adoc
	DESTINATION "extras/EsotericSoftwareSpine"
)
