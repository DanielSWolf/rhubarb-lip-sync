cmake_minimum_required(VERSION 3.3)
project(LipSync)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Find Boost libraries
set(Boost_USE_STATIC_LIBS ON) # Use static libs
set(Boost_USE_MULTITHREADED ON) # Enable multithreading support
set(Boost_USE_STATIC_RUNTIME ON) # Use static C++ runtime
find_package(Boost REQUIRED COMPONENTS filesystem locale system)
include_directories(${Boost_INCLUDE_DIRS})

set(SOURCE_FILES src/main.cpp src/audio_input/WaveFileReader.cpp src/audio_input/WaveFileReader.h src/audio_input/ChannelDownmixer.cpp src/audio_input/ChannelDownmixer.h src/audio_input/AudioStream.h src/audio_input/SampleRateConverter.cpp src/audio_input/SampleRateConverter.h src/audio_input/wave_file_writing.cpp src/audio_input/wave_file_writing.h src/audio_input/io_tools.h src/platform_tools.cpp src/platform_tools.h src/phone_extraction.cpp src/phone_extraction.h src/Phone.cpp src/Phone.h src/centiseconds.cpp src/centiseconds.h src/tools.cpp src/tools.h src/Shape.cpp src/Shape.h src/mouth_animation.cpp src/mouth_animation.h)

include_directories("lib/sphinxbase-5prealpha-2015-08-05/include" "lib/pocketsphinx-5prealpha-2015-08-05/include" "lib/cppformat")
FILE(GLOB_RECURSE SPHINX_BASE "lib/sphinxbase-5prealpha-2015-08-05/src/libsphinxbase/*.c")
FILE(GLOB POCKETSPHINX "lib/pocketsphinx-5prealpha-2015-08-05/src/libpocketsphinx/*.c")
FILE(GLOB CPPFORMAT "lib/cppformat/*.cc")

add_executable(LipSync ${SOURCE_FILES} ${SPHINX_BASE} ${POCKETSPHINX} ${CPPFORMAT})
target_link_libraries(LipSync ${Boost_LIBRARIES})

function(copy_after_build sourceGlob relativeTargetDirectory)
	# Set `sourcePaths`
	file(GLOB sourcePaths "${sourceGlob}")
	
	foreach(sourcePath ${sourcePaths})
		# Set `fileName`
		get_filename_component(fileName "${sourcePath}" NAME)
		
		# Set `targetPath`
		set(targetPath "${CMAKE_BINARY_DIR}/${relativeTargetDirectory}/${fileName}")
		
		add_custom_command(TARGET LipSync POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy "${sourcePath}" "${targetPath}"
			COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
		)
	endforeach()
endfunction()

# Copy resource files
set(modelDir "${CMAKE_SOURCE_DIR}/lib/pocketsphinx-5prealpha-2015-08-05/model")
copy_after_build("${modelDir}/en-us/en-us-phone.lm.bin" "res/sphinx")
copy_after_build("${modelDir}/en-us/en-us/*" "res/sphinx/acoustic_model")
