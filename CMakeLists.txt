cmake_minimum_required(VERSION 3.3)
project(LipSync)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp src/audio_input/WaveFileReader.cpp src/audio_input/WaveFileReader.h src/audio_input/ChannelDownmixer.cpp src/audio_input/ChannelDownmixer.h src/audio_input/AudioStream.h src/audio_input/SampleRateConverter.cpp src/audio_input/SampleRateConverter.h src/audio_input/16kHzMonoStream.cpp src/audio_input/16kHzMonoStream.h src/audio_input/WaveFileWriter.cpp src/audio_input/WaveFileWriter.h src/audio_input/IOTools.h)

include_directories("lib/sphinxbase-5prealpha-2015-08-05/include" "lib/pocketsphinx-5prealpha-2015-08-05/include" "lib/cppformat")
FILE(GLOB_RECURSE SPHINX_BASE "lib/sphinxbase-5prealpha-2015-08-05/src/libsphinxbase/*.c")
FILE(GLOB POCKETSPHINX "lib/pocketsphinx-5prealpha-2015-08-05/src/libpocketsphinx/*.c")
FILE(GLOB CPPFORMAT "lib/cppformat/*.cc")

add_executable(LipSync ${SOURCE_FILES} ${SPHINX_BASE} ${POCKETSPHINX} ${CPPFORMAT})