cmake_minimum_required(VERSION 3.3)

# Support legacy OS X versions
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7" CACHE STRING "Minimum OS X deployment version")

set(appName "Rhubarb Lip Sync")
set(appVersion "0.1.0-alpha")

project(${appName})

# Enable C++14
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

# Make sure Xcode uses libc++ instead of libstdc++, allowing us to use the C++14 standard library prior to OS X 10.9
if("${CMAKE_GENERATOR}" STREQUAL "Xcode")
	add_compile_options(-stdlib=libc++)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
endif()

# Use static run-time
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_options(/MT$<$<CONFIG:Debug>:d>)
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Define flags variables for later use
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(enableWarningsFlags "-Wall;-Wextra")
	set(disableWarningsFlags "-w")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(enableWarningsFlags "/W4")
	set(disableWarningsFlags "/W0")
endif()

# Enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define libraries

# ... Boost
set(Boost_USE_STATIC_LIBS ON) # Use static libs
set(Boost_USE_MULTITHREADED ON) # Enable multithreading support
set(Boost_USE_STATIC_RUNTIME ON) # Use static C++ runtime
find_package(Boost REQUIRED COMPONENTS filesystem locale system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# ... C++ Format
include_directories(SYSTEM "lib/cppformat")
FILE(GLOB cppFormatFiles "lib/cppformat/*.cc")
add_library(cppFormat ${cppFormatFiles})
target_compile_options(cppFormat PRIVATE ${disableWarningsFlags})
set_target_properties(cppFormat PROPERTIES FOLDER lib)

# ... sphinxbase
include_directories(SYSTEM "lib/sphinxbase-redefine/include" "lib/sphinxbase-5prealpha-2015-08-05/include")
FILE(GLOB_RECURSE sphinxbaseFiles "lib/sphinxbase-5prealpha-2015-08-05/src/libsphinxbase/*.c")
add_library(sphinxbase ${sphinxbaseFiles})
target_compile_options(sphinxbase PRIVATE ${disableWarningsFlags})
set_target_properties(sphinxbase PROPERTIES FOLDER lib)

# ... PocketSphinx
include_directories(SYSTEM "lib/pocketsphinx-redefine/include" "lib/pocketsphinx-5prealpha-2015-08-05/include")
FILE(GLOB pocketSphinxFiles "lib/pocketsphinx-5prealpha-2015-08-05/src/libpocketsphinx/*.c")
add_library(pocketSphinx ${pocketSphinxFiles})
target_link_libraries(pocketSphinx sphinxbase)
target_compile_options(pocketSphinx PRIVATE ${disableWarningsFlags})
set_target_properties(pocketSphinx PROPERTIES FOLDER lib)

# ... TCLAP
include_directories(SYSTEM "lib/tclap-1.2.1/include")

# Define executable
include_directories("src" "src/audio_input")
configure_file(src/app_info.cpp.in src/app_info.cpp ESCAPE_QUOTES)
set(SOURCE_FILES
	${CMAKE_CURRENT_BINARY_DIR}/src/app_info.cpp
	src/main.cpp
	src/Phone.cpp
	src/Shape.cpp
	src/centiseconds.cpp
	src/mouth_animation.cpp
	src/phone_extraction.cpp
	src/platform_tools.cpp
	src/tools.cpp
	src/audio_input/ChannelDownmixer.cpp
	src/audio_input/SampleRateConverter.cpp
	src/audio_input/WaveFileReader.cpp
	src/audio_input/wave_file_writing.cpp
)
add_executable(rhubarb ${SOURCE_FILES})
target_link_libraries(rhubarb ${Boost_LIBRARIES} cppFormat sphinxbase pocketSphinx)
target_compile_options(rhubarb PUBLIC ${enableWarningsFlags})

# Copy resource files
include(tools.cmake)
set(modelDir "${CMAKE_SOURCE_DIR}/lib/pocketsphinx-5prealpha-2015-08-05/model")
copy_after_build("${modelDir}/en-us/en-us-phone.lm.bin" "res/sphinx")
copy_after_build("${modelDir}/en-us/en-us/*" "res/sphinx/acoustic_model")
