cmake_minimum_required(VERSION 3.2)

include("../appInfo.cmake")

# Support legacy OS X versions
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8" CACHE STRING "Minimum OS X deployment version")

project(${appName})

# Enable C++14
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

# Enable POSIX threads
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lpthread")
endif()

# Make sure Xcode uses libc++ instead of libstdc++, allowing us to use the C++14 standard library prior to OS X 10.9
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
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

	# Disable warning C4456: declaration of '...' hides previous local declaration
	# I'm doing that on purpose.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458")

	# Assume UTF-8 encoding for source files and encode string constants in UTF-8
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

# Use UTF-8 throughout
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_options("/utf-8")
endif()

if(${UNIX})
	add_definitions(-DHAVE_UNISTD_H)
endif()

# Enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define libraries

# ... Boost
set(Boost_USE_STATIC_LIBS ON) # Use static libs
set(Boost_USE_MULTITHREADED ON) # Enable multithreading support
set(Boost_USE_STATIC_RUNTIME ON) # Use static C++ runtime
find_package(Boost 1.54 REQUIRED COMPONENTS filesystem locale system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES}) # Just about every project needs Boost

# ... C++ Format
FILE(GLOB cppFormatFiles "lib/cppformat/*.cc")
add_library(cppFormat ${cppFormatFiles})
target_include_directories(cppFormat SYSTEM PUBLIC "lib/cppformat")
target_compile_options(cppFormat PRIVATE ${disableWarningsFlags})
set_target_properties(cppFormat PROPERTIES FOLDER lib)

# ... sphinxbase
FILE(GLOB_RECURSE sphinxbaseFiles "lib/sphinxbase-rev13216/src/libsphinxbase/*.c")
add_library(sphinxbase ${sphinxbaseFiles})
target_include_directories(sphinxbase SYSTEM PUBLIC
	"lib/sphinxbase-rev13216/include"
	"lib/sphinxbase-rev13216/src"
	"lib/sphinx_config"
)
target_compile_options(sphinxbase PRIVATE ${disableWarningsFlags})
target_compile_definitions(sphinxbase PUBLIC __SPHINXBASE_EXPORT_H__=1 SPHINXBASE_EXPORT=) # Compile as static lib
set_target_properties(sphinxbase PROPERTIES FOLDER lib)

# ... PocketSphinx
FILE(GLOB pocketSphinxFiles "lib/pocketsphinx-rev13216/src/libpocketsphinx/*.c")
add_library(pocketSphinx ${pocketSphinxFiles})
target_include_directories(pocketSphinx SYSTEM PUBLIC
	"lib/pocketsphinx-rev13216/include"
	"lib/pocketsphinx-rev13216/src/libpocketsphinx"
)
target_link_libraries(pocketSphinx sphinxbase)
target_compile_options(pocketSphinx PRIVATE ${disableWarningsFlags})
target_compile_definitions(pocketSphinx PUBLIC __POCKETSPHINX_EXPORT_H__=1 POCKETSPHINX_EXPORT=) # Compile as static lib
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

# ... WebRTC
set(webRtcFiles
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/cross_correlation.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/division_operations.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/downsample_fast.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/energy.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/get_scaling_square.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/min_max_operations.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/resample_48khz.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/resample_by_2_internal.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/resample_fractional.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/spl_init.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/spl_inl.c
	lib/webrtc-8d2248ff/webrtc/common_audio/signal_processing/vector_scaling_operations.c
	lib/webrtc-8d2248ff/webrtc/common_audio/vad/vad_core.c
	lib/webrtc-8d2248ff/webrtc/common_audio/vad/vad_filterbank.c
	lib/webrtc-8d2248ff/webrtc/common_audio/vad/vad_gmm.c
	lib/webrtc-8d2248ff/webrtc/common_audio/vad/vad_sp.c
	lib/webrtc-8d2248ff/webrtc/common_audio/vad/webrtc_vad.c
)
add_library(webRtc ${webRtcFiles})
target_include_directories(webRtc SYSTEM PUBLIC "lib/webrtc-8d2248ff")
target_compile_options(webRtc PRIVATE ${disableWarningsFlags})
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  target_compile_options(webRtc PRIVATE -pthread -lpthread)
endif()
if (NOT WIN32)
	target_compile_definitions(webRtc PRIVATE WEBRTC_POSIX)
endif()
set_target_properties(webRtc PROPERTIES FOLDER lib)

# ... whereami
add_library(whereami lib/whereami/src/whereami.c)
target_include_directories(whereami SYSTEM PUBLIC "lib/whereami/src")
set_target_properties(whereami PROPERTIES FOLDER lib)

# ... Flite
set(fliteFiles
	lib/flite-1.4/lang/cmulex/cmu_lex.c
	lib/flite-1.4/lang/cmulex/cmu_lex_data.c
	lib/flite-1.4/lang/cmulex/cmu_lex_entries.c
	lib/flite-1.4/lang/cmulex/cmu_lts_model.c
	lib/flite-1.4/lang/cmulex/cmu_lts_rules.c
	lib/flite-1.4/lang/cmulex/cmu_postlex.c
	lib/flite-1.4/lang/usenglish/us_aswd.c
	lib/flite-1.4/lang/usenglish/us_dur_stats.c
	lib/flite-1.4/lang/usenglish/us_durz_cart.c
	lib/flite-1.4/lang/usenglish/us_expand.c
	lib/flite-1.4/lang/usenglish/us_f0_model.c
	lib/flite-1.4/lang/usenglish/us_f0lr.c
	lib/flite-1.4/lang/usenglish/us_ffeatures.c
	lib/flite-1.4/lang/usenglish/us_gpos.c
	lib/flite-1.4/lang/usenglish/us_int_accent_cart.c
	lib/flite-1.4/lang/usenglish/us_int_tone_cart.c
	lib/flite-1.4/lang/usenglish/us_nums_cart.c
	lib/flite-1.4/lang/usenglish/us_phoneset.c
	lib/flite-1.4/lang/usenglish/us_phrasing_cart.c
	lib/flite-1.4/lang/usenglish/us_pos_cart.c
	lib/flite-1.4/lang/usenglish/us_text.c
	lib/flite-1.4/lang/usenglish/usenglish.c
	lib/flite-1.4/src/audio/au_none.c
	lib/flite-1.4/src/audio/au_streaming.c
	lib/flite-1.4/src/audio/audio.c
	lib/flite-1.4/src/hrg/cst_ffeature.c
	lib/flite-1.4/src/hrg/cst_item.c
	lib/flite-1.4/src/hrg/cst_relation.c
	lib/flite-1.4/src/hrg/cst_utterance.c
	lib/flite-1.4/src/lexicon/cst_lexicon.c
	lib/flite-1.4/src/lexicon/cst_lts.c
	lib/flite-1.4/src/regex/cst_regex.c
	lib/flite-1.4/src/regex/regexp.c
	lib/flite-1.4/src/speech/cst_lpcres.c
	lib/flite-1.4/src/speech/cst_track.c
	lib/flite-1.4/src/speech/cst_wave.c
	lib/flite-1.4/src/speech/cst_wave_io.c
	lib/flite-1.4/src/speech/cst_wave_utils.c
	lib/flite-1.4/src/speech/rateconv.c
	lib/flite-1.4/src/stats/cst_cart.c
	lib/flite-1.4/src/synth/cst_ffeatures.c
	lib/flite-1.4/src/synth/cst_phoneset.c
	lib/flite-1.4/src/synth/cst_synth.c
	lib/flite-1.4/src/synth/cst_utt_utils.c
	lib/flite-1.4/src/synth/cst_voice.c
	lib/flite-1.4/src/synth/flite.c
	lib/flite-1.4/src/utils/cst_alloc.c
	lib/flite-1.4/src/utils/cst_endian.c
	lib/flite-1.4/src/utils/cst_error.c
	lib/flite-1.4/src/utils/cst_features.c
	lib/flite-1.4/src/utils/cst_file_stdio.c
	lib/flite-1.4/src/utils/cst_string.c
	lib/flite-1.4/src/utils/cst_tokenstream.c
	lib/flite-1.4/src/utils/cst_val.c
	lib/flite-1.4/src/utils/cst_val_const.c
	lib/flite-1.4/src/utils/cst_val_user.c
	lib/flite-1.4/src/utils/cst_val_user.c
)
add_library(flite ${fliteFiles})
target_include_directories(flite SYSTEM PUBLIC
	"lib/flite-1.4/include"
	"lib/flite-1.4"
)
target_compile_options(flite PRIVATE ${disableWarningsFlags})
set_target_properties(flite PROPERTIES FOLDER lib)

# ... UTF8-CPP
add_library(utfcpp
	lib/header-only.c
	lib/utfcpp-2.3.5/source/utf8.h
)
target_include_directories(utfcpp SYSTEM PUBLIC "lib/utfcpp-2.3.5/source")
target_compile_options(utfcpp PRIVATE ${disableWarningsFlags})
set_target_properties(utfcpp PROPERTIES FOLDER lib)

# ... utf8proc
add_library(utf8proc
	lib/utf8proc-2a2f97e1/utf8proc.c
	lib/utf8proc-2a2f97e1/utf8proc.h
)
target_include_directories(utf8proc SYSTEM PUBLIC "lib/utf8proc-2a2f97e1")
target_compile_options(utf8proc PRIVATE ${disableWarningsFlags})
set_target_properties(utf8proc PROPERTIES FOLDER lib)

# Define Rhubarb libraries

include_directories("src")

# ... rhubarb-animation
add_library(rhubarb-animation
	src/animation/animationRules.cpp
	src/animation/animationRules.h
	src/animation/mouthAnimation.cpp
	src/animation/mouthAnimation.h
	src/animation/pauseAnimation.cpp
	src/animation/pauseAnimation.h
	src/animation/roughAnimation.cpp
	src/animation/roughAnimation.h
	src/animation/ShapeRule.cpp
	src/animation/ShapeRule.h
	src/animation/shapeShorthands.h
	src/animation/staticSegments.cpp
	src/animation/staticSegments.h
	src/animation/targetShapeSet.cpp
	src/animation/targetShapeSet.h
	src/animation/timingOptimization.cpp
	src/animation/timingOptimization.h
	src/animation/tweening.cpp
	src/animation/tweening.h
)
target_include_directories(rhubarb-animation PRIVATE "src/animation")
target_link_libraries(rhubarb-animation
	rhubarb-core
	rhubarb-logging
	rhubarb-time
)

# ... rhubarb-audio
add_library(rhubarb-audio
	src/audio/AudioClip.cpp
	src/audio/AudioClip.h
	src/audio/AudioSegment.cpp
	src/audio/AudioSegment.h
	src/audio/DcOffset.cpp
	src/audio/DcOffset.h
	src/audio/ioTools.h
	src/audio/processing.cpp
	src/audio/processing.h
	src/audio/SampleRateConverter.cpp
	src/audio/SampleRateConverter.h
	src/audio/voiceActivityDetection.cpp
	src/audio/voiceActivityDetection.h
	src/audio/WaveFileReader.cpp
	src/audio/WaveFileReader.h
	src/audio/waveFileWriting.cpp
	src/audio/waveFileWriting.h
)
target_include_directories(rhubarb-audio PRIVATE "src/audio")
target_link_libraries(rhubarb-audio
	webRtc
	rhubarb-logging
	rhubarb-time
	rhubarb-tools
)

# ... rhubarb-core
configure_file(src/core/appInfo.cpp.in appInfo.cpp ESCAPE_QUOTES)
add_library(rhubarb-core
	${CMAKE_CURRENT_BINARY_DIR}/appInfo.cpp
	src/core/appInfo.h
	src/core/Phone.cpp
	src/core/Phone.h
	src/core/Shape.cpp
	src/core/Shape.h
)
target_include_directories(rhubarb-core PRIVATE "src/core")
target_link_libraries(rhubarb-core
	rhubarb-tools
)

# ... rhubarb-exporters
add_library(rhubarb-exporters
	src/exporters/Exporter.h
	src/exporters/exporterTools.cpp
	src/exporters/exporterTools.h
	src/exporters/JsonExporter.cpp
	src/exporters/JsonExporter.h
	src/exporters/TsvExporter.cpp
	src/exporters/TsvExporter.h
	src/exporters/XmlExporter.cpp
	src/exporters/XmlExporter.h
)
target_include_directories(rhubarb-exporters PRIVATE "src/exporters")
target_link_libraries(rhubarb-exporters
	rhubarb-animation
	rhubarb-core
	rhubarb-time
)

# ... rhubarb-lib
add_library(rhubarb-lib
	src/lib/rhubarbLib.cpp
	src/lib/rhubarbLib.h
)
target_include_directories(rhubarb-lib PRIVATE "src/lib")
target_link_libraries(rhubarb-lib
	rhubarb-animation
	rhubarb-audio
	rhubarb-core
	rhubarb-recognition
	rhubarb-time
	rhubarb-tools
)

# ... rhubarb-logging
add_library(rhubarb-logging
	src/logging/Entry.cpp
	src/logging/Entry.h
	src/logging/Formatter.h
	src/logging/formatters.cpp
	src/logging/formatters.h
	src/logging/Level.cpp
	src/logging/Level.h
	src/logging/logging.cpp
	src/logging/logging.h
	src/logging/Sink.h
	src/logging/sinks.cpp
	src/logging/sinks.h
)
target_include_directories(rhubarb-logging PRIVATE "src/logging")
target_link_libraries(rhubarb-logging
	rhubarb-tools
)

# ... rhubarb-recognition
add_library(rhubarb-recognition
	src/recognition/g2p.cpp
	src/recognition/g2p.h
	src/recognition/languageModels.cpp
	src/recognition/languageModels.h
	src/recognition/phoneRecognition.cpp
	src/recognition/phoneRecognition.h
	src/recognition/tokenization.cpp
	src/recognition/tokenization.h
)
target_include_directories(rhubarb-recognition PRIVATE "src/recognition")
target_link_libraries(rhubarb-recognition
	flite
	pocketSphinx
	rhubarb-audio
	rhubarb-core
	rhubarb-logging
)

# ... rhubarb-time
add_library(rhubarb-time
	src/time/BoundedTimeline.h
	src/time/centiseconds.cpp
	src/time/centiseconds.h
	src/time/ContinuousTimeline.h
	src/time/Timed.h
	src/time/timedLogging.h
	src/time/Timeline.h
	src/time/TimeRange.cpp
	src/time/TimeRange.h
)
target_include_directories(rhubarb-time PRIVATE "src/time")
target_link_libraries(rhubarb-time
	cppFormat
	rhubarb-logging
)

# ... rhubarb-tools
add_library(rhubarb-tools
	src/tools/array.h
	src/tools/EnumConverter.h
	src/tools/exceptions.cpp
	src/tools/exceptions.h
	src/tools/Lazy.h
	src/tools/nextCombination.h
	src/tools/NiceCmdLineOutput.cpp
	src/tools/NiceCmdLineOutput.h
	src/tools/ObjectPool.h
	src/tools/pairs.h
	src/tools/parallel.h
	src/tools/platformTools.cpp
	src/tools/platformTools.h
	src/tools/ProgressBar.cpp
	src/tools/ProgressBar.h
	src/tools/stringTools.cpp
	src/tools/stringTools.h
	src/tools/TablePrinter.cpp
	src/tools/TablePrinter.h
	src/tools/textFiles.cpp
	src/tools/textFiles.h
	src/tools/tools.cpp
	src/tools/tools.h
	src/tools/tupleHash.h
)
target_include_directories(rhubarb-tools PRIVATE "src/tools")
target_link_libraries(rhubarb-tools
	cppFormat
	whereami
	utfcpp
	utf8proc
)

# Define Rhubarb executable
add_executable(rhubarb
	src/rhubarb/main.cpp
	src/rhubarb/ExportFormat.cpp
	src/rhubarb/ExportFormat.h
	src/rhubarb/semanticEntries.cpp
	src/rhubarb/semanticEntries.h
	src/rhubarb/sinks.cpp
	src/rhubarb/sinks.h
)
target_include_directories(rhubarb PUBLIC "src/rhubarb")
target_link_libraries(rhubarb
	rhubarb-exporters
	rhubarb-lib
)
target_compile_options(rhubarb PUBLIC ${enableWarningsFlags})

# Define test project
#include_directories("${gtest_SOURCE_DIR}/include")
set(TEST_FILES
	tests/stringToolsTests.cpp
	tests/TimelineTests.cpp
	tests/BoundedTimelineTests.cpp
	tests/ContinuousTimelineTests.cpp
	tests/pairsTests.cpp
	tests/tokenizationTests.cpp
	tests/g2pTests.cpp
	tests/LazyTests.cpp
)
add_executable(runTests ${TEST_FILES})
target_link_libraries(runTests
	gtest
	gmock
	gmock_main
	rhubarb-recognition
	rhubarb-time
)

# Copies the specified files in a post-build event, then installs them
function(copy_and_install sourceGlob relativeTargetDirectory)
	# Set `sourcePaths`
	file(GLOB sourcePaths "${sourceGlob}")
	
	foreach(sourcePath ${sourcePaths})
		if(NOT IS_DIRECTORY ${sourcePath})
			# Set `fileName`
			get_filename_component(fileName "${sourcePath}" NAME)

			# Copy file during build
			add_custom_command(TARGET rhubarb POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy "${sourcePath}" "$<TARGET_FILE_DIR:rhubarb>/${relativeTargetDirectory}/${fileName}"
				COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
			)

			# Install file
			install(
				FILES "${sourcePath}"
				DESTINATION "${relativeTargetDirectory}"
			)
		endif()
	endforeach()
endfunction()

copy_and_install("lib/pocketsphinx-rev13216/model/en-us/*" "res/sphinx")
copy_and_install("lib/cmusphinx-en-us-5.2/*" "res/sphinx/acoustic-model")

install(
	TARGETS rhubarb
	RUNTIME
	DESTINATION .
)
