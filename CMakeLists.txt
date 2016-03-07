cmake_minimum_required(VERSION 3.3)

# Support legacy OS X versions
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7" CACHE STRING "Minimum OS X deployment version")

set(appName "Rhubarb Lip Sync")
set(appVersionMajor 0)
set(appVersionMinor 1)
set(appVersionPatch 0)
set(appVersionSuffix "")
set(appVersion "${appVersionMajor}.${appVersionMinor}.${appVersionPatch}${appVersionSuffix}")

project(${appName})

# Enable C++14
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
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

# Set global flags and define flags variables for later use
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(enableWarningsFlags "-Wall;-Wextra")
	set(disableWarningsFlags "-w")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(enableWarningsFlags "/W4")
	set(disableWarningsFlags "/W0")
	
	# Boost.Log causes this warning
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4714")
endif()

# Enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define libraries

# ... Boost
set(Boost_USE_STATIC_LIBS ON) # Use static libs
set(Boost_USE_MULTITHREADED ON) # Enable multithreading support
set(Boost_USE_STATIC_RUNTIME ON) # Use static C++ runtime
find_package(Boost REQUIRED COMPONENTS filesystem locale system log date_time thread chrono)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# ... C++ Format
include_directories(SYSTEM "lib/cppformat")
FILE(GLOB cppFormatFiles "lib/cppformat/*.cc")
add_library(cppFormat ${cppFormatFiles})
target_compile_options(cppFormat PRIVATE ${disableWarningsFlags})
set_target_properties(cppFormat PROPERTIES FOLDER lib)

# ... sphinxbase
include_directories(SYSTEM "lib/sphinxbase-5prealpha-2015-08-05/include")
FILE(GLOB_RECURSE sphinxbaseFiles "lib/sphinxbase-5prealpha-2015-08-05/src/libsphinxbase/*.c")
add_library(sphinxbase ${sphinxbaseFiles})
target_compile_options(sphinxbase PRIVATE ${disableWarningsFlags})
set_target_properties(sphinxbase PROPERTIES FOLDER lib)

# ... PocketSphinx
include_directories(SYSTEM "lib/pocketsphinx-5prealpha-2015-08-05/include" "lib/pocketsphinx-5prealpha-2015-08-05/src/libpocketsphinx")
FILE(GLOB pocketSphinxFiles "lib/pocketsphinx-5prealpha-2015-08-05/src/libpocketsphinx/*.c")
add_library(pocketSphinx ${pocketSphinxFiles})
target_link_libraries(pocketSphinx sphinxbase)
target_compile_options(pocketSphinx PRIVATE ${disableWarningsFlags})
set_target_properties(pocketSphinx PROPERTIES FOLDER lib)

# ... TCLAP
include_directories(SYSTEM "lib/tclap-1.2.1/include")

# ... Google Test
add_subdirectory("lib/googletest")
set_target_properties(gmock PROPERTIES FOLDER lib)
set_target_properties(gmock_main PROPERTIES FOLDER lib)
set_target_properties(gtest PROPERTIES FOLDER lib)
set_target_properties(gtest_main PROPERTIES FOLDER lib)

# ... GSL
include_directories(SYSTEM "lib/gsl/include")

# Define executable
include_directories("src" "src/audio_input")
configure_file(src/appInfo.cpp.in src/appInfo.cpp ESCAPE_QUOTES)
set(SOURCE_FILES
	${CMAKE_CURRENT_BINARY_DIR}/src/appInfo.cpp
	src/main.cpp
	src/Phone.cpp
	src/Shape.cpp
	src/centiseconds.cpp
	src/mouthAnimation.cpp
	src/phoneExtraction.cpp
	src/platformTools.cpp
	src/tools.cpp
	src/audioInput/AudioStream.cpp
	src/audioInput/SampleRateConverter.cpp
	src/audioInput/WaveFileReader.cpp
	src/audioInput/waveFileWriting.cpp
	src/stringTools.cpp
	src/NiceCmdLineOutput.cpp
	src/TablePrinter.cpp
	src/ProgressBar.cpp
	src/logging.cpp
)
add_executable(rhubarb ${SOURCE_FILES})
target_link_libraries(rhubarb ${Boost_LIBRARIES} cppFormat sphinxbase pocketSphinx)
target_compile_options(rhubarb PUBLIC ${enableWarningsFlags})

# Define test project
#include_directories("${gtest_SOURCE_DIR}/include")
set(TEST_FILES
	tests/stringToolsTests.cpp
	src/stringTools.cpp
)
add_executable(runTests ${TEST_FILES})
target_link_libraries(runTests gtest gmock gmock_main)

set(CPACK_PACKAGE_NAME ${appName})
string(REPLACE " " "-" CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	set(CPACK_SYSTEM_NAME "OSX")
endif()
set(CPACK_PACKAGE_VERSION_MAJOR ${appVersionMajor})
set(CPACK_PACKAGE_VERSION_MINOR ${appVersionMinor})
set(CPACK_PACKAGE_VERSION_PATCH ${appVersionPatch})
set(CPACK_PACKAGE_VERSION ${appVersion})
set(CPACK_GENERATOR ZIP)

# Copy resource files at build time; install them at package time
include(tools.cmake)
set(modelDir "${CMAKE_SOURCE_DIR}/lib/pocketsphinx-5prealpha-2015-08-05/model")
copy_and_install("${modelDir}/en-us/*" "res/sphinx")
copy_and_install("${modelDir}/en-us/en-us/*" "res/sphinx/acoustic-model")

install(
	TARGETS rhubarb
	RUNTIME
	DESTINATION .
)
install(
	FILES README.md LICENSE.md VERSION.md
	DESTINATION .
)

include(CPack)
