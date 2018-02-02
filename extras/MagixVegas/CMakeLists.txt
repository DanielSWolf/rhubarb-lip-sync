cmake_minimum_required(VERSION 3.2)

set(vegasFiles
	"Debug Rhubarb.cs"
	"Debug Rhubarb.cs.config"
	"Import Rhubarb.cs"
	"Import Rhubarb.cs.config"
	"README.md"
)

install(
	FILES ${vegasFiles}
	DESTINATION "extras/MagixVegas"
)
