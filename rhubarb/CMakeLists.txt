cmake_minimum_required(VERSION 3.10)

# Parse app info
file(READ "../app-info.toml" tomlContent)
string(REGEX MATCH "appName *= *\"[^\"]+\"" appName "${tomlContent}")
string(REGEX REPLACE ".*\"([^\"]+)\"" "\\1" appName "${appName}")
string(REGEX MATCH "appVersion *= *\"[^\"]+\"" appVersion "${tomlContent}")
string(REGEX REPLACE ".*\"([^\"]+)\"" "\\1" appVersion "${appVersion}")

project("${appName}")

# Store compiler information
if(
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang"
)
    set(compilerIsGccOrClang true)
else()
    set(compilerIsGccOrClang false)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(compilerIsMsvc true)
else()
    set(compilerIsMsvc false)
endif()

# Support legacy OS X versions
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable POSIX threads
if(compilerIsGccOrClang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Use static run-time
if(compilerIsMsvc)
    add_compile_options(/MT$<$<CONFIG:Debug>:d>)
endif()

# Set global warning flags and define flags variables for later use
if(compilerIsGccOrClang)
    set(enableWarningsFlags "-Wall;-Wextra")
    set(disableWarningsFlags "-w")
elseif(compilerIsMsvc)
    set(enableWarningsFlags "/W4")
    set(disableWarningsFlags "/W0")

    # Disable warning C4456: declaration of '...' hides previous local declaration
    # I'm doing that on purpose.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458")
endif()

# Assume UTF-8 encoding for source files and encode string constants in UTF-8
if(compilerIsMsvc)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
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
set(Boost_NO_BOOST_CMAKE ON) # Workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/18865
find_package(Boost 1.54 REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES}) # Just about every project needs Boost

# ... C++ Format
file(GLOB cppFormatFiles "lib/cppformat/*.cc")
add_library(cppFormat ${cppFormatFiles})
target_include_directories(cppFormat SYSTEM PUBLIC "lib/cppformat")
target_compile_options(cppFormat PRIVATE ${disableWarningsFlags})
set_target_properties(cppFormat PROPERTIES FOLDER lib)

# ... sphinxbase
file(GLOB_RECURSE sphinxbaseFiles "lib/sphinxbase-rev13216/src/libsphinxbase/*.c")
add_library(sphinxbase ${sphinxbaseFiles})
target_include_directories(
    sphinxbase
    SYSTEM
    PUBLIC "lib/sphinxbase-rev13216/include" "lib/sphinxbase-rev13216/src" "lib/sphinx_config"
)
target_compile_options(sphinxbase PRIVATE ${disableWarningsFlags})
target_compile_definitions(sphinxbase PUBLIC __SPHINXBASE_EXPORT_H__=1 SPHINXBASE_EXPORT=) # Compile as static lib
set_target_properties(sphinxbase PROPERTIES FOLDER lib)

# ... PocketSphinx
file(GLOB pocketSphinxFiles "lib/pocketsphinx-rev13216/src/libpocketsphinx/*.c")
add_library(pocketSphinx ${pocketSphinxFiles})
target_include_directories(
    pocketSphinx
    SYSTEM
    PUBLIC "lib/pocketsphinx-rev13216/include" "lib/pocketsphinx-rev13216/src/libpocketsphinx"
)
target_link_libraries(pocketSphinx sphinxbase)
target_compile_options(pocketSphinx PRIVATE ${disableWarningsFlags})
target_compile_definitions(pocketSphinx PUBLIC __POCKETSPHINX_EXPORT_H__=1 POCKETSPHINX_EXPORT=) # Compile as static lib
set_target_properties(pocketSphinx PROPERTIES FOLDER lib)

# ... TCLAP
include_directories(SYSTEM "lib/tclap-1.2.1/include")

# ... Google Test
set(INSTALL_GTEST OFF) # Prevent library files from ending up in our artifacts
add_subdirectory("lib/googletest")
target_compile_options(gmock PRIVATE ${disableWarningsFlags})
set_target_properties(gmock PROPERTIES FOLDER lib)
target_compile_options(gmock_main PRIVATE ${disableWarningsFlags})
set_target_properties(gmock_main PROPERTIES FOLDER lib)
target_compile_options(gtest PRIVATE ${disableWarningsFlags})
set_target_properties(gtest PROPERTIES FOLDER lib)
target_compile_options(gtest_main PRIVATE ${disableWarningsFlags})
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
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(webRtc PRIVATE -pthread -lpthread)
endif()
if(NOT WIN32)
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
target_include_directories(flite SYSTEM PUBLIC "lib/flite-1.4/include" "lib/flite-1.4")
target_compile_options(flite PRIVATE ${disableWarningsFlags})
set_target_properties(flite PROPERTIES FOLDER lib)

# ... UTF8-CPP
add_library(utfcpp lib/header-only.c lib/utfcpp-2.3.5/source/utf8.h)
target_include_directories(utfcpp SYSTEM PUBLIC "lib/utfcpp-2.3.5/source")
target_compile_options(utfcpp PRIVATE ${disableWarningsFlags})
set_target_properties(utfcpp PROPERTIES FOLDER lib)

# ... utf8proc
add_library(utf8proc lib/utf8proc-2.2.0/utf8proc.c lib/utf8proc-2.2.0/utf8proc.h)
target_include_directories(utf8proc SYSTEM PUBLIC "lib/utf8proc-2.2.0")
target_compile_options(utf8proc PRIVATE ${disableWarningsFlags})
target_compile_definitions(utf8proc PUBLIC UTF8PROC_STATIC=1) # Compile as static lib
set_target_properties(utf8proc PROPERTIES FOLDER lib)

# ... Ogg
add_library(
    ogg
    lib/ogg-1.3.3/include/ogg/ogg.h
    lib/ogg-1.3.3/src/bitwise.c
    lib/ogg-1.3.3/src/framing.c
)
target_include_directories(ogg SYSTEM PUBLIC "lib/ogg-1.3.3/include")
target_compile_options(ogg PRIVATE ${disableWarningsFlags})
set_target_properties(ogg PROPERTIES FOLDER lib)

# ... Vorbis
add_library(
    vorbis
    lib/vorbis-1.3.6/include/vorbis/vorbisfile.h
    lib/vorbis-1.3.6/lib/bitrate.c
    lib/vorbis-1.3.6/lib/block.c
    lib/vorbis-1.3.6/lib/codebook.c
    lib/vorbis-1.3.6/lib/envelope.c
    lib/vorbis-1.3.6/lib/floor0.c
    lib/vorbis-1.3.6/lib/floor1.c
    lib/vorbis-1.3.6/lib/info.c
    lib/vorbis-1.3.6/lib/lpc.c
    lib/vorbis-1.3.6/lib/lsp.c
    lib/vorbis-1.3.6/lib/mapping0.c
    lib/vorbis-1.3.6/lib/mdct.c
    lib/vorbis-1.3.6/lib/psy.c
    lib/vorbis-1.3.6/lib/registry.c
    lib/vorbis-1.3.6/lib/res0.c
    lib/vorbis-1.3.6/lib/sharedbook.c
    lib/vorbis-1.3.6/lib/smallft.c
    lib/vorbis-1.3.6/lib/synthesis.c
    lib/vorbis-1.3.6/lib/vorbisfile.c
    lib/vorbis-1.3.6/lib/window.c
)
target_include_directories(vorbis SYSTEM PUBLIC "lib/vorbis-1.3.6/include")
target_link_libraries(vorbis ogg)
target_compile_options(vorbis PRIVATE ${disableWarningsFlags})
set_target_properties(vorbis PROPERTIES FOLDER lib)

# Define Rhubarb libraries

include_directories("src")

# ... rhubarb-animation
add_library(
    rhubarb-animation
    src/animation/animation-rules.cpp
    src/animation/animation-rules.h
    src/animation/mouth-animation.cpp
    src/animation/mouth-animation.h
    src/animation/pause-animation.cpp
    src/animation/pause-animation.h
    src/animation/rough-animation.cpp
    src/animation/rough-animation.h
    src/animation/shape-rule.cpp
    src/animation/shape-rule.h
    src/animation/shape-shorthands.h
    src/animation/static-segments.cpp
    src/animation/static-segments.h
    src/animation/target-shape-set.cpp
    src/animation/target-shape-set.h
    src/animation/timing-optimization.cpp
    src/animation/timing-optimization.h
    src/animation/tweening.cpp
    src/animation/tweening.h
)
target_include_directories(rhubarb-animation PRIVATE "src/animation")
target_link_libraries(rhubarb-animation rhubarb-core rhubarb-logging rhubarb-time)

# ... rhubarb-audio
add_library(
    rhubarb-audio
    src/audio/audio-clip.cpp
    src/audio/audio-clip.h
    src/audio/audio-file-reading.cpp
    src/audio/audio-file-reading.h
    src/audio/audio-segment.cpp
    src/audio/audio-segment.h
    src/audio/dc-offset.cpp
    src/audio/dc-offset.h
    src/audio/io-tools.h
    src/audio/ogg-vorbis-file-reader.cpp
    src/audio/ogg-vorbis-file-reader.h
    src/audio/processing.cpp
    src/audio/processing.h
    src/audio/sample-rate-converter.cpp
    src/audio/sample-rate-converter.h
    src/audio/voice-activity-detection.cpp
    src/audio/voice-activity-detection.h
    src/audio/wave-file-reader.cpp
    src/audio/wave-file-reader.h
    src/audio/wave-file-writing.cpp
    src/audio/wave-file-writing.h
)
target_include_directories(rhubarb-audio PRIVATE "src/audio")
target_link_libraries(
    rhubarb-audio
    webRtc
    vorbis
    rhubarb-logging
    rhubarb-time
    rhubarb-tools
)

# ... rhubarb-core
configure_file(src/core/app-info.cpp.in app-info.cpp)
add_library(
    rhubarb-core
    ${CMAKE_CURRENT_BINARY_DIR}/app-info.cpp
    src/core/app-info.h
    src/core/phone.cpp
    src/core/phone.h
    src/core/shape.cpp
    src/core/shape.h
)
target_include_directories(rhubarb-core PRIVATE "src/core")
target_link_libraries(rhubarb-core rhubarb-tools)

# ... rhubarb-exporters
add_library(
    rhubarb-exporters
    src/exporters/dat-exporter.cpp
    src/exporters/dat-exporter.h
    src/exporters/exporter.h
    src/exporters/exporter-tools.cpp
    src/exporters/exporter-tools.h
    src/exporters/json-exporter.cpp
    src/exporters/json-exporter.h
    src/exporters/tsv-exporter.cpp
    src/exporters/tsv-exporter.h
    src/exporters/xml-exporter.cpp
    src/exporters/xml-exporter.h
)
target_include_directories(rhubarb-exporters PRIVATE "src/exporters")
target_link_libraries(rhubarb-exporters rhubarb-animation rhubarb-core rhubarb-time)

# ... rhubarb-lib
add_library(rhubarb-lib src/lib/rhubarb-lib.cpp src/lib/rhubarb-lib.h)
target_include_directories(rhubarb-lib PRIVATE "src/lib")
target_link_libraries(
    rhubarb-lib
    rhubarb-animation
    rhubarb-audio
    rhubarb-core
    rhubarb-recognition
    rhubarb-time
    rhubarb-tools
)

# ... rhubarb-logging
add_library(
    rhubarb-logging
    src/logging/entry.cpp
    src/logging/entry.h
    src/logging/formatter.h
    src/logging/formatters.cpp
    src/logging/formatters.h
    src/logging/level.cpp
    src/logging/level.h
    src/logging/logging.cpp
    src/logging/logging.h
    src/logging/sink.h
    src/logging/sinks.cpp
    src/logging/sinks.h
)
target_include_directories(rhubarb-logging PRIVATE "src/logging")
target_link_libraries(rhubarb-logging rhubarb-tools)

# ... rhubarb-recognition
add_library(
    rhubarb-recognition
    src/recognition/g2p.cpp
    src/recognition/g2p.h
    src/recognition/language-models.cpp
    src/recognition/language-models.h
    src/recognition/phonetic-recognizer.cpp
    src/recognition/phonetic-recognizer.h
    src/recognition/pocket-sphinx-recognizer.cpp
    src/recognition/pocket-sphinx-recognizer.h
    src/recognition/pocket-sphinx-tools.cpp
    src/recognition/pocket-sphinx-tools.h
    src/recognition/recognizer.h
    src/recognition/tokenization.cpp
    src/recognition/tokenization.h
)
target_include_directories(rhubarb-recognition PRIVATE "src/recognition")
target_link_libraries(
    rhubarb-recognition
    flite
    pocketSphinx
    rhubarb-audio
    rhubarb-core
    rhubarb-logging
)

# ... rhubarb-time
add_library(
    rhubarb-time
    src/time/bounded-timeline.h
    src/time/centiseconds.cpp
    src/time/centiseconds.h
    src/time/continuous-timeline.h
    src/time/timed.h
    src/time/timed-logging.h
    src/time/timeline.h
    src/time/time-range.cpp
    src/time/time-range.h
)
target_include_directories(rhubarb-time PRIVATE "src/time")
target_link_libraries(rhubarb-time cppFormat rhubarb-logging)

# ... rhubarb-tools
add_library(
    rhubarb-tools
    src/tools/array.h
    src/tools/enum-converter.h
    src/tools/exceptions.cpp
    src/tools/exceptions.h
    src/tools/file-tools.cpp
    src/tools/file-tools.h
    src/tools/lazy.h
    src/tools/next-combination.h
    src/tools/nice-cmd-line-output.cpp
    src/tools/nice-cmd-line-output.h
    src/tools/object-pool.h
    src/tools/pairs.h
    src/tools/parallel.h
    src/tools/platform-tools.cpp
    src/tools/platform-tools.h
    src/tools/progress.cpp
    src/tools/progress.h
    src/tools/progress-bar.cpp
    src/tools/progress-bar.h
    src/tools/string-tools.cpp
    src/tools/string-tools.h
    src/tools/table-printer.cpp
    src/tools/table-printer.h
    src/tools/text-files.cpp
    src/tools/text-files.h
    src/tools/tools.cpp
    src/tools/tools.h
    src/tools/tuple-hash.h
)
target_include_directories(rhubarb-tools PRIVATE "src/tools")
target_link_libraries(rhubarb-tools cppFormat whereami utfcpp utf8proc)

# Define Rhubarb executable
add_executable(
    rhubarb
    src/rhubarb/main.cpp
    src/rhubarb/export-format.cpp
    src/rhubarb/export-format.h
    src/rhubarb/recognizer-type.cpp
    src/rhubarb/recognizer-type.h
    src/rhubarb/semantic-entries.cpp
    src/rhubarb/semantic-entries.h
    src/rhubarb/sinks.cpp
    src/rhubarb/sinks.h
)
target_include_directories(rhubarb PUBLIC "src/rhubarb")
target_link_libraries(rhubarb rhubarb-exporters rhubarb-lib)
target_compile_options(rhubarb PUBLIC ${enableWarningsFlags})

# Define test project
#include_directories("${gtest_SOURCE_DIR}/include")
set(TEST_FILES
    tests/string-tools-tests.cpp
    tests/timeline-tests.cpp
    tests/bounded-timeline-tests.cpp
    tests/continuous-timeline-tests.cpp
    tests/pairs-tests.cpp
    tests/tokenization-tests.cpp
    tests/g2p-tests.cpp
    tests/lazy-tests.cpp
    tests/wave-file-reader-tests.cpp
)
add_executable(runTests ${TEST_FILES})
target_link_libraries(
    runTests
    gtest
    gmock
    gmock_main
    rhubarb-recognition
    rhubarb-time
    rhubarb-audio
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
            add_custom_command(
                TARGET rhubarb
                POST_BUILD
                COMMAND
                    ${CMAKE_COMMAND} -E copy "${sourcePath}"
                    "$<TARGET_FILE_DIR:rhubarb>/${relativeTargetDirectory}/${fileName}"
                COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
            )

            # Install file
            install(FILES "${sourcePath}" DESTINATION "${relativeTargetDirectory}")
        endif()
    endforeach()
endfunction()

# Copies the specified files in a post-build event
function(copy sourceGlob relativeTargetDirectory)
    # Set `sourcePaths`
    file(GLOB sourcePaths "${sourceGlob}")

    foreach(sourcePath ${sourcePaths})
        if(NOT IS_DIRECTORY ${sourcePath})
            # Set `fileName`
            get_filename_component(fileName "${sourcePath}" NAME)

            # Copy file during build
            add_custom_command(
                TARGET rhubarb
                POST_BUILD
                COMMAND
                    ${CMAKE_COMMAND} -E copy "${sourcePath}"
                    "$<TARGET_FILE_DIR:rhubarb>/${relativeTargetDirectory}/${fileName}"
                COMMENT "Creating '${relativeTargetDirectory}/${fileName}'"
            )
        endif()
    endforeach()
endfunction()

copy_and_install("lib/pocketsphinx-rev13216/model/en-us/*" "res/sphinx")
copy_and_install("lib/cmusphinx-en-us-5.2/*" "res/sphinx/acoustic-model")
install(TARGETS rhubarb RUNTIME DESTINATION .)
