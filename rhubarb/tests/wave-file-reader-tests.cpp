#include <gmock/gmock.h>

#include <filesystem>

#include "audio/wave-file-reader.h"
#include "tools/platform-tools.h"

using namespace testing;
using std::filesystem::path;

path getResourcesDirectory() {
    path p = getBinDirectory();

    // Search up for resources directory
    while (true) {
        path resourcesDirectory = p / "tests/resources";
        if (std::filesystem::exists(resourcesDirectory)) return resourcesDirectory;

        p = p.parent_path();
    }
}

TEST(getWaveFormatInfo, float32FromAudacity) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-float32-audacity.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Float32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 88);
}

TEST(getWaveFormatInfo, float32FromAudition) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-float32-audition.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Float32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 92);
}

TEST(getWaveFormatInfo, float32FromFfmpeg) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-float32-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Float32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 114);
}

TEST(getWaveFormatInfo, float32FromSoundforge) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-float32-soundforge.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Float32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, float64FromFfmpeg) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-float64-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Float64);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 8);
    EXPECT_EQ(formatInfo.dataOffset, 114);
}

TEST(getWaveFormatInfo, int16FromAudacity) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int16-audacity.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int16);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 2);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, int16FromAudition) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int16-audition.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int16);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 2);
    EXPECT_EQ(formatInfo.dataOffset, 92);
}

TEST(getWaveFormatInfo, int16FromFfmpeg) {
    auto formatInfo = getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int16-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int16);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 2);
    EXPECT_EQ(formatInfo.dataOffset, 78);
}

TEST(getWaveFormatInfo, int16FromSoundforge) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int16-soundforge.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int16);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 2);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, int24FromAudacity) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int24-audacity.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int24);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 3);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, int24FromAudition) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int24-audition.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int24);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 3);
    EXPECT_EQ(formatInfo.dataOffset, 92);
}

TEST(getWaveFormatInfo, int24FromFfmpeg) {
    auto formatInfo = getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int24-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int24);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 3);
    EXPECT_EQ(formatInfo.dataOffset, 102);
}

TEST(getWaveFormatInfo, int24FromSoundforge) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int24-soundforge.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int24);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 3);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, int32FromFfmpeg) {
    auto formatInfo = getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int32-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 102);
}

TEST(getWaveFormatInfo, int32FromSoundforge) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-int32-soundforge.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::Int32);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 4);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}

TEST(getWaveFormatInfo, uint8FromAudition) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-uint8-audition.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::UInt8);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 1);
    EXPECT_EQ(formatInfo.dataOffset, 92);
}

TEST(getWaveFormatInfo, uint8FromFfmpeg) {
    auto formatInfo = getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-uint8-ffmpeg.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::UInt8);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 1);
    EXPECT_EQ(formatInfo.dataOffset, 78);
}

TEST(getWaveFormatInfo, uint8FromSoundforge) {
    auto formatInfo =
        getWaveFormatInfo(getResourcesDirectory() / "sine-triangle-uint8-soundforge.wav");
    EXPECT_EQ(formatInfo.frameRate, 48000);
    EXPECT_EQ(formatInfo.frameCount, 480000);
    EXPECT_EQ(formatInfo.channelCount, 2);
    EXPECT_EQ(formatInfo.sampleFormat, SampleFormat::UInt8);
    EXPECT_EQ(formatInfo.bytesPerFrame, 2 * 1);
    EXPECT_EQ(formatInfo.dataOffset, 44);
}
