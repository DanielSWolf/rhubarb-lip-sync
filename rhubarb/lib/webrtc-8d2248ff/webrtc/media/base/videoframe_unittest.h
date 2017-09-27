/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_BASE_VIDEOFRAME_UNITTEST_H_
#define WEBRTC_MEDIA_BASE_VIDEOFRAME_UNITTEST_H_

#include <algorithm>
#include <memory>
#include <string>

#include "libyuv/convert.h"
#include "libyuv/convert_from.h"
#include "libyuv/planar_functions.h"
#include "libyuv/rotate.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/common_video/rotation.h"
#include "webrtc/media/base/testutils.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/media/base/videoframe.h"
#include "webrtc/test/testsupport/fileutils.h"

#if defined(_MSC_VER)
#define ALIGN16(var) __declspec(align(16)) var
#else
#define ALIGN16(var) var __attribute__((aligned(16)))
#endif

#define kImageFilename "media/faces.1280x720_P420"
#define kYuvExtension "yuv"
#define kJpeg420Filename "media/faces_I420"
#define kJpeg422Filename "media/faces_I422"
#define kJpeg444Filename "media/faces_I444"
#define kJpeg411Filename "media/faces_I411"
#define kJpeg400Filename "media/faces_I400"
#define kJpegExtension "jpg"

// Generic test class for testing various video frame implementations.
template <class T>
class VideoFrameTest : public testing::Test {
 public:
  VideoFrameTest() : repeat_(1) {}

 protected:
  static const int kWidth = 1280;
  static const int kHeight = 720;
  static const int kAlignment = 16;
  static const int kMinWidthAll = 1;  // Constants for ConstructYUY2AllSizes.
  static const int kMinHeightAll = 1;
  static const int kMaxWidthAll = 17;
  static const int kMaxHeightAll = 23;

  // Load a video frame from disk.
  bool LoadFrameNoRepeat(T* frame) {
    int save_repeat = repeat_;  // This LoadFrame disables repeat.
    repeat_ = 1;
    bool success = LoadFrame(LoadSample(kImageFilename, kYuvExtension).get(),
                             cricket::FOURCC_I420,
                             kWidth, kHeight, frame);
    repeat_ = save_repeat;
    return success;
  }

  bool LoadFrame(const std::string& filename,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 T* frame) {
    return LoadFrame(filename, format, width, height, width, abs(height),
                     webrtc::kVideoRotation_0, frame);
  }
  bool LoadFrame(const std::string& filename,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 int dw,
                 int dh,
                 webrtc::VideoRotation rotation,
                 T* frame) {
    std::unique_ptr<rtc::MemoryStream> ms(LoadSample(filename));
    return LoadFrame(ms.get(), format, width, height, dw, dh, rotation, frame);
  }
  // Load a video frame from a memory stream.
  bool LoadFrame(rtc::MemoryStream* ms,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 T* frame) {
    return LoadFrame(ms, format, width, height, width, abs(height),
                     webrtc::kVideoRotation_0, frame);
  }
  bool LoadFrame(rtc::MemoryStream* ms,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 int dw,
                 int dh,
                 webrtc::VideoRotation rotation,
                 T* frame) {
    if (!ms) {
      return false;
    }
    size_t data_size;
    bool ret = ms->GetSize(&data_size);
    EXPECT_TRUE(ret);
    if (ret) {
      ret = LoadFrame(reinterpret_cast<uint8_t*>(ms->GetBuffer()), data_size,
                      format, width, height, dw, dh, rotation, frame);
    }
    return ret;
  }
  // Load a frame from a raw buffer.
  bool LoadFrame(uint8_t* sample,
                 size_t sample_size,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 T* frame) {
    return LoadFrame(sample, sample_size, format, width, height, width,
                     abs(height), webrtc::kVideoRotation_0, frame);
  }
  bool LoadFrame(uint8_t* sample,
                 size_t sample_size,
                 uint32_t format,
                 int32_t width,
                 int32_t height,
                 int dw,
                 int dh,
                 webrtc::VideoRotation rotation,
                 T* frame) {
    bool ret = false;
    for (int i = 0; i < repeat_; ++i) {
      ret = frame->Init(format, width, height, dw, dh,
                        sample, sample_size, 0, rotation);
    }
    return ret;
  }

  std::unique_ptr<rtc::MemoryStream> LoadSample(const std::string& filename,
                                                const std::string& extension) {
    rtc::Pathname path(webrtc::test::ResourcePath(filename, extension));
    std::unique_ptr<rtc::FileStream> fs(
        rtc::Filesystem::OpenFile(path, "rb"));
    if (!fs.get()) {
      LOG(LS_ERROR) << "Could not open test file path: " << path.pathname()
                    << " from current dir "
                    << rtc::Filesystem::GetCurrentDirectory().pathname();
      return NULL;
    }

    char buf[4096];
    std::unique_ptr<rtc::MemoryStream> ms(
        new rtc::MemoryStream());
    rtc::StreamResult res = Flow(fs.get(), buf, sizeof(buf), ms.get());
    if (res != rtc::SR_SUCCESS) {
      LOG(LS_ERROR) << "Could not load test file path: " << path.pathname();
      return NULL;
    }

    return ms;
  }

  bool DumpSample(const std::string& filename, const void* buffer, int size) {
    rtc::Pathname path(filename);
    std::unique_ptr<rtc::FileStream> fs(
        rtc::Filesystem::OpenFile(path, "wb"));
    if (!fs.get()) {
      return false;
    }

    return (fs->Write(buffer, size, NULL, NULL) == rtc::SR_SUCCESS);
  }

  // Create a test image in the desired color space.
  // The image is a checkerboard pattern with 63x63 squares, which allows
  // I420 chroma artifacts to easily be seen on the square boundaries.
  // The pattern is { { green, orange }, { blue, purple } }
  // There is also a gradient within each square to ensure that the luma
  // values are handled properly.
  std::unique_ptr<rtc::MemoryStream> CreateYuv422Sample(uint32_t fourcc,
                                                        uint32_t width,
                                                        uint32_t height) {
    int y1_pos, y2_pos, u_pos, v_pos;
    if (!GetYuv422Packing(fourcc, &y1_pos, &y2_pos, &u_pos, &v_pos)) {
      return NULL;
    }

    std::unique_ptr<rtc::MemoryStream> ms(
        new rtc::MemoryStream);
    int awidth = (width + 1) & ~1;
    int size = awidth * 2 * height;
    if (!ms->ReserveSize(size)) {
      return NULL;
    }
    for (uint32_t y = 0; y < height; ++y) {
      for (int x = 0; x < awidth; x += 2) {
        uint8_t quad[4];
        quad[y1_pos] = (x % 63 + y % 63) + 64;
        quad[y2_pos] = ((x + 1) % 63 + y % 63) + 64;
        quad[u_pos] = ((x / 63) & 1) ? 192 : 64;
        quad[v_pos] = ((y / 63) & 1) ? 192 : 64;
        ms->Write(quad, sizeof(quad), NULL, NULL);
      }
    }
    return ms;
  }

  // Create a test image for YUV 420 formats with 12 bits per pixel.
  std::unique_ptr<rtc::MemoryStream> CreateYuvSample(uint32_t width,
                                                     uint32_t height,
                                                     uint32_t bpp) {
    std::unique_ptr<rtc::MemoryStream> ms(
        new rtc::MemoryStream);
    if (!ms->ReserveSize(width * height * bpp / 8)) {
      return NULL;
    }

    for (uint32_t i = 0; i < width * height * bpp / 8; ++i) {
      uint8_t value = ((i / 63) & 1) ? 192 : 64;
      ms->Write(&value, sizeof(value), NULL, NULL);
    }
    return ms;
  }

  std::unique_ptr<rtc::MemoryStream> CreateRgbSample(uint32_t fourcc,
                                                     uint32_t width,
                                                     uint32_t height) {
    int r_pos, g_pos, b_pos, bytes;
    if (!GetRgbPacking(fourcc, &r_pos, &g_pos, &b_pos, &bytes)) {
      return NULL;
    }

    std::unique_ptr<rtc::MemoryStream> ms(
        new rtc::MemoryStream);
    if (!ms->ReserveSize(width * height * bytes)) {
      return NULL;
    }

    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        uint8_t rgb[4] = {255, 255, 255, 255};
        rgb[r_pos] = ((x / 63) & 1) ? 224 : 32;
        rgb[g_pos] = (x % 63 + y % 63) + 96;
        rgb[b_pos] = ((y / 63) & 1) ? 224 : 32;
        ms->Write(rgb, bytes, NULL, NULL);
      }
    }
    return ms;
  }

  // Simple conversion routines to verify the optimized VideoFrame routines.
  // Converts from the specified colorspace to I420.
  std::unique_ptr<T> ConvertYuv422(const rtc::MemoryStream* ms,
                                   uint32_t fourcc,
                                   uint32_t width,
                                   uint32_t height) {
    int y1_pos, y2_pos, u_pos, v_pos;
    if (!GetYuv422Packing(fourcc, &y1_pos, &y2_pos, &u_pos, &v_pos)) {
      return nullptr;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> buffer(
        new rtc::RefCountedObject<webrtc::I420Buffer>(width, height));

    buffer->SetToBlack();

    const uint8_t* start = reinterpret_cast<const uint8_t*>(ms->GetBuffer());
    int awidth = (width + 1) & ~1;
    int stride_y = buffer->StrideY();
    int stride_u = buffer->StrideU();
    int stride_v = buffer->StrideV();
    uint8_t* plane_y = buffer->MutableDataY();
    uint8_t* plane_u = buffer->MutableDataU();
    uint8_t* plane_v = buffer->MutableDataV();
    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; x += 2) {
        const uint8_t* quad1 = start + (y * awidth + x) * 2;
        plane_y[stride_y * y + x] = quad1[y1_pos];
        if ((x + 1) < width) {
          plane_y[stride_y * y + x + 1] = quad1[y2_pos];
        }
        if ((y & 1) == 0) {
          const uint8_t* quad2 = quad1 + awidth * 2;
          if ((y + 1) >= height) {
            quad2 = quad1;
          }
          plane_u[stride_u * (y / 2) + x / 2] =
              (quad1[u_pos] + quad2[u_pos] + 1) / 2;
          plane_v[stride_v * (y / 2) + x / 2] =
              (quad1[v_pos] + quad2[v_pos] + 1) / 2;
        }
      }
    }
    return std::unique_ptr<T>(new T(buffer, 0, webrtc::kVideoRotation_0));
  }

  // Convert RGB to 420.
  // A negative height inverts the image.
  std::unique_ptr<T> ConvertRgb(const rtc::MemoryStream* ms,
                                uint32_t fourcc,
                                int32_t width,
                                int32_t height) {
    int r_pos, g_pos, b_pos, bytes;
    if (!GetRgbPacking(fourcc, &r_pos, &g_pos, &b_pos, &bytes)) {
      return nullptr;
    }
    int pitch = width * bytes;
    const uint8_t* start = reinterpret_cast<const uint8_t*>(ms->GetBuffer());
    if (height < 0) {
      height = -height;
      start = start + pitch * (height - 1);
      pitch = -pitch;
    }
    rtc::scoped_refptr<webrtc::I420Buffer> buffer(
        new rtc::RefCountedObject<webrtc::I420Buffer>(width, height));

    buffer->SetToBlack();

    int stride_y = buffer->StrideY();
    int stride_u = buffer->StrideU();
    int stride_v = buffer->StrideV();
    uint8_t* plane_y = buffer->MutableDataY();
    uint8_t* plane_u = buffer->MutableDataU();
    uint8_t* plane_v = buffer->MutableDataV();
    for (int32_t y = 0; y < height; y += 2) {
      for (int32_t x = 0; x < width; x += 2) {
        const uint8_t* rgb[4];
        uint8_t yuv[4][3];
        rgb[0] = start + y * pitch + x * bytes;
        rgb[1] = rgb[0] + ((x + 1) < width ? bytes : 0);
        rgb[2] = rgb[0] + ((y + 1) < height ? pitch : 0);
        rgb[3] = rgb[2] + ((x + 1) < width ? bytes : 0);
        for (size_t i = 0; i < 4; ++i) {
          ConvertRgbPixel(rgb[i][r_pos], rgb[i][g_pos], rgb[i][b_pos],
                          &yuv[i][0], &yuv[i][1], &yuv[i][2]);
        }
        plane_y[stride_y * y + x] = yuv[0][0];
        if ((x + 1) < width) {
          plane_y[stride_y * y + x + 1] = yuv[1][0];
        }
        if ((y + 1) < height) {
          plane_y[stride_y * (y + 1) + x] = yuv[2][0];
          if ((x + 1) < width) {
            plane_y[stride_y * (y + 1) + x + 1] = yuv[3][0];
          }
        }
        plane_u[stride_u * (y / 2) + x / 2] =
            (yuv[0][1] + yuv[1][1] + yuv[2][1] + yuv[3][1] + 2) / 4;
        plane_v[stride_v * (y / 2) + x / 2] =
            (yuv[0][2] + yuv[1][2] + yuv[2][2] + yuv[3][2] + 2) / 4;
      }
    }
    return std::unique_ptr<T>(new T(buffer, 0, webrtc::kVideoRotation_0));
  }

  // Simple and slow RGB->YUV conversion. From NTSC standard, c/o Wikipedia.
  void ConvertRgbPixel(uint8_t r,
                       uint8_t g,
                       uint8_t b,
                       uint8_t* y,
                       uint8_t* u,
                       uint8_t* v) {
    *y = static_cast<int>(.257 * r + .504 * g + .098 * b) + 16;
    *u = static_cast<int>(-.148 * r - .291 * g + .439 * b) + 128;
    *v = static_cast<int>(.439 * r - .368 * g - .071 * b) + 128;
  }

  bool GetYuv422Packing(uint32_t fourcc,
                        int* y1_pos,
                        int* y2_pos,
                        int* u_pos,
                        int* v_pos) {
    if (fourcc == cricket::FOURCC_YUY2) {
      *y1_pos = 0; *u_pos = 1; *y2_pos = 2; *v_pos = 3;
    } else if (fourcc == cricket::FOURCC_UYVY) {
      *u_pos = 0; *y1_pos = 1; *v_pos = 2; *y2_pos = 3;
    } else {
      return false;
    }
    return true;
  }

  bool GetRgbPacking(uint32_t fourcc,
                     int* r_pos,
                     int* g_pos,
                     int* b_pos,
                     int* bytes) {
    if (fourcc == cricket::FOURCC_RAW) {
      *r_pos = 0; *g_pos = 1; *b_pos = 2; *bytes = 3;  // RGB in memory.
    } else if (fourcc == cricket::FOURCC_24BG) {
      *r_pos = 2; *g_pos = 1; *b_pos = 0; *bytes = 3;  // BGR in memory.
    } else if (fourcc == cricket::FOURCC_ABGR) {
      *r_pos = 0; *g_pos = 1; *b_pos = 2; *bytes = 4;  // RGBA in memory.
    } else if (fourcc == cricket::FOURCC_BGRA) {
      *r_pos = 1; *g_pos = 2; *b_pos = 3; *bytes = 4;  // ARGB in memory.
    } else if (fourcc == cricket::FOURCC_ARGB) {
      *r_pos = 2; *g_pos = 1; *b_pos = 0; *bytes = 4;  // BGRA in memory.
    } else {
      return false;
    }
    return true;
  }

  // Comparison functions for testing.
  static bool IsNull(const cricket::VideoFrame& frame) {
    return !frame.video_frame_buffer();
  }

  static bool IsSize(const cricket::VideoFrame& frame,
                     int width,
                     int height) {
    return !IsNull(frame) && frame.video_frame_buffer()->StrideY() >= width &&
           frame.video_frame_buffer()->StrideU() >= width / 2 &&
           frame.video_frame_buffer()->StrideV() >= width / 2 &&
           frame.width() == width && frame.height() == height;
  }

  static bool IsPlaneEqual(const std::string& name,
                           const uint8_t* plane1,
                           uint32_t pitch1,
                           const uint8_t* plane2,
                           uint32_t pitch2,
                           uint32_t width,
                           uint32_t height,
                           int max_error) {
    const uint8_t* r1 = plane1;
    const uint8_t* r2 = plane2;
    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        if (abs(static_cast<int>(r1[x] - r2[x])) > max_error) {
          LOG(LS_INFO) << "IsPlaneEqual(" << name << "): pixel["
                       << x << "," << y << "] differs: "
                       << static_cast<int>(r1[x]) << " vs "
                       << static_cast<int>(r2[x]);
          return false;
        }
      }
      r1 += pitch1;
      r2 += pitch2;
    }
    return true;
  }

  static bool IsEqual(const cricket::VideoFrame& frame,
                      int width,
                      int height,
                      int64_t time_stamp,
                      const uint8_t* y,
                      uint32_t ypitch,
                      const uint8_t* u,
                      uint32_t upitch,
                      const uint8_t* v,
                      uint32_t vpitch,
                      int max_error) {
    return IsSize(frame, width, height) && frame.GetTimeStamp() == time_stamp &&
           IsPlaneEqual("y", frame.video_frame_buffer()->DataY(),
                        frame.video_frame_buffer()->StrideY(), y, ypitch,
                        static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height), max_error) &&
           IsPlaneEqual("u", frame.video_frame_buffer()->DataU(),
                        frame.video_frame_buffer()->StrideU(), u, upitch,
                        static_cast<uint32_t>((width + 1) / 2),
                        static_cast<uint32_t>((height + 1) / 2), max_error) &&
           IsPlaneEqual("v", frame.video_frame_buffer()->DataV(),
                        frame.video_frame_buffer()->StrideV(), v, vpitch,
                        static_cast<uint32_t>((width + 1) / 2),
                        static_cast<uint32_t>((height + 1) / 2), max_error);
  }

  static bool IsEqual(const cricket::VideoFrame& frame1,
                      const cricket::VideoFrame& frame2,
                      int max_error) {
    return IsEqual(frame1,
                   frame2.width(), frame2.height(),
                   frame2.GetTimeStamp(),
                   frame2.video_frame_buffer()->DataY(),
                   frame2.video_frame_buffer()->StrideY(),
                   frame2.video_frame_buffer()->DataU(),
                   frame2.video_frame_buffer()->StrideU(),
                   frame2.video_frame_buffer()->DataV(),
                   frame2.video_frame_buffer()->StrideV(),
                   max_error);
  }

  static bool IsEqualWithCrop(const cricket::VideoFrame& frame1,
                              const cricket::VideoFrame& frame2,
                              int hcrop, int vcrop, int max_error) {
    return frame1.width() <= frame2.width() &&
           frame1.height() <= frame2.height() &&
           IsEqual(frame1,
                   frame2.width() - hcrop * 2,
                   frame2.height() - vcrop * 2,
                   frame2.GetTimeStamp(),
                   frame2.video_frame_buffer()->DataY()
                       + vcrop * frame2.video_frame_buffer()->StrideY()
                       + hcrop,
                   frame2.video_frame_buffer()->StrideY(),
                   frame2.video_frame_buffer()->DataU()
                       + vcrop * frame2.video_frame_buffer()->StrideU() / 2
                       + hcrop / 2,
                   frame2.video_frame_buffer()->StrideU(),
                   frame2.video_frame_buffer()->DataV()
                       + vcrop * frame2.video_frame_buffer()->StrideV() / 2
                       + hcrop / 2,
                   frame2.video_frame_buffer()->StrideV(),
                   max_error);
  }

  static bool IsBlack(const cricket::VideoFrame& frame) {
    return !IsNull(frame) &&
           *frame.video_frame_buffer()->DataY() <= 16 &&
           *frame.video_frame_buffer()->DataU() == 128 &&
           *frame.video_frame_buffer()->DataV() == 128;
  }

  ////////////////////////
  // Construction tests //
  ////////////////////////

  // Test constructing an image from a I420 buffer.
  void ConstructI420() {
    T frame;
    EXPECT_TRUE(IsNull(frame));
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuvSample(kWidth, kHeight, 12));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_I420,
                          kWidth, kHeight, &frame));

    const uint8_t* y = reinterpret_cast<uint8_t*>(ms.get()->GetBuffer());
    const uint8_t* u = y + kWidth * kHeight;
    const uint8_t* v = u + kWidth * kHeight / 4;
    EXPECT_TRUE(IsEqual(frame, kWidth, kHeight, 0, y, kWidth, u,
                        kWidth / 2, v, kWidth / 2, 0));
  }

  // Test constructing an image from a YV12 buffer.
  void ConstructYV12() {
    T frame;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuvSample(kWidth, kHeight, 12));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YV12,
                          kWidth, kHeight, &frame));

    const uint8_t* y = reinterpret_cast<uint8_t*>(ms.get()->GetBuffer());
    const uint8_t* v = y + kWidth * kHeight;
    const uint8_t* u = v + kWidth * kHeight / 4;
    EXPECT_TRUE(IsEqual(frame, kWidth, kHeight, 0, y, kWidth, u,
                        kWidth / 2, v, kWidth / 2, 0));
  }

  // Test constructing an image from a I422 buffer.
  void ConstructI422() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    size_t buf_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[buf_size + kAlignment]);
    uint8_t* y = ALIGNP(buf.get(), kAlignment);
    uint8_t* u = y + kWidth * kHeight;
    uint8_t* v = u + (kWidth / 2) * kHeight;
    EXPECT_EQ(0, libyuv::I420ToI422(frame1.video_frame_buffer()->DataY(),
                                    frame1.video_frame_buffer()->StrideY(),
                                    frame1.video_frame_buffer()->DataU(),
                                    frame1.video_frame_buffer()->StrideU(),
                                    frame1.video_frame_buffer()->DataV(),
                                    frame1.video_frame_buffer()->StrideV(),
                                    y, kWidth,
                                    u, kWidth / 2,
                                    v, kWidth / 2,
                                    kWidth, kHeight));
    EXPECT_TRUE(LoadFrame(y, buf_size, cricket::FOURCC_I422,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 1));
  }

  // Test constructing an image from a YUY2 buffer.
  void ConstructYuy2() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    size_t buf_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[buf_size + kAlignment]);
    uint8_t* yuy2 = ALIGNP(buf.get(), kAlignment);
    EXPECT_EQ(0, libyuv::I420ToYUY2(frame1.video_frame_buffer()->DataY(),
                                    frame1.video_frame_buffer()->StrideY(),
                                    frame1.video_frame_buffer()->DataU(),
                                    frame1.video_frame_buffer()->StrideU(),
                                    frame1.video_frame_buffer()->DataV(),
                                    frame1.video_frame_buffer()->StrideV(),
                                    yuy2, kWidth * 2,
                                    kWidth, kHeight));
    EXPECT_TRUE(LoadFrame(yuy2, buf_size, cricket::FOURCC_YUY2,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
  }

  // Test constructing an image from a YUY2 buffer with buffer unaligned.
  void ConstructYuy2Unaligned() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    size_t buf_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[buf_size + kAlignment + 1]);
    uint8_t* yuy2 = ALIGNP(buf.get(), kAlignment) + 1;
    EXPECT_EQ(0, libyuv::I420ToYUY2(frame1.video_frame_buffer()->DataY(),
                                    frame1.video_frame_buffer()->StrideY(),
                                    frame1.video_frame_buffer()->DataU(),
                                    frame1.video_frame_buffer()->StrideU(),
                                    frame1.video_frame_buffer()->DataV(),
                                    frame1.video_frame_buffer()->StrideV(),
                                    yuy2, kWidth * 2,
                                    kWidth, kHeight));
    EXPECT_TRUE(LoadFrame(yuy2, buf_size, cricket::FOURCC_YUY2,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
  }

  // Test constructing an image from a wide YUY2 buffer.
  // Normal is 1280x720.  Wide is 12800x72
  void ConstructYuy2Wide() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth * 10, kHeight / 10));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertYuv422(ms.get(), cricket::FOURCC_YUY2,
                                              kWidth * 10, kHeight / 10);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2,
                          kWidth * 10, kHeight / 10, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 0));
  }

  // Test constructing an image from a UYVY buffer.
  void ConstructUyvy() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_UYVY, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertYuv422(ms.get(), cricket::FOURCC_UYVY,
                                              kWidth, kHeight);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_UYVY,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 0));
  }

  // Test constructing an image from a random buffer.
  // We are merely verifying that the code succeeds and is free of crashes.
  void ConstructM420() {
    T frame;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuvSample(kWidth, kHeight, 12));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_M420,
                          kWidth, kHeight, &frame));
  }

  void ConstructNV21() {
    T frame;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuvSample(kWidth, kHeight, 12));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_NV21,
                          kWidth, kHeight, &frame));
  }

  void ConstructNV12() {
    T frame;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuvSample(kWidth, kHeight, 12));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_NV12,
                          kWidth, kHeight, &frame));
  }

  // Test constructing an image from a ABGR buffer
  // Due to rounding, some pixels may differ slightly from the VideoFrame impl.
  void ConstructABGR() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_ABGR, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ABGR,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ABGR,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from a ARGB buffer
  // Due to rounding, some pixels may differ slightly from the VideoFrame impl.
  void ConstructARGB() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_ARGB, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ARGB,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ARGB,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from a wide ARGB buffer
  // Normal is 1280x720.  Wide is 12800x72
  void ConstructARGBWide() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_ARGB, kWidth * 10, kHeight / 10));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ARGB,
                                           kWidth * 10, kHeight / 10);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ARGB,
                          kWidth * 10, kHeight / 10, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from an BGRA buffer.
  // Due to rounding, some pixels may differ slightly from the VideoFrame impl.
  void ConstructBGRA() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_BGRA, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_BGRA,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_BGRA,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from a 24BG buffer.
  // Due to rounding, some pixels may differ slightly from the VideoFrame impl.
  void Construct24BG() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_24BG, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_24BG,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_24BG,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from a raw RGB buffer.
  // Due to rounding, some pixels may differ slightly from the VideoFrame impl.
  void ConstructRaw() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_RAW, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_RAW,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_RAW,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(*frame1, frame2, 2));
  }

  // Test constructing an image from a RGB565 buffer
  void ConstructRGB565() {
    T frame1, frame2;
    size_t out_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment]);
    uint8_t* out = ALIGNP(outbuf.get(), kAlignment);
    T frame;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    EXPECT_EQ(out_size, frame1.ConvertToRgbBuffer(cricket::FOURCC_RGBP,
                                                 out,
                                                 out_size, kWidth * 2));
    EXPECT_TRUE(LoadFrame(out, out_size, cricket::FOURCC_RGBP,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 20));
  }

  // Test constructing an image from a ARGB1555 buffer
  void ConstructARGB1555() {
    T frame1, frame2;
    size_t out_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment]);
    uint8_t* out = ALIGNP(outbuf.get(), kAlignment);
    T frame;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    EXPECT_EQ(out_size, frame1.ConvertToRgbBuffer(cricket::FOURCC_RGBO,
                                                 out,
                                                 out_size, kWidth * 2));
    EXPECT_TRUE(LoadFrame(out, out_size, cricket::FOURCC_RGBO,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 20));
  }

  // Test constructing an image from a ARGB4444 buffer
  void ConstructARGB4444() {
    T frame1, frame2;
    size_t out_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment]);
    uint8_t* out = ALIGNP(outbuf.get(), kAlignment);
    T frame;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    EXPECT_EQ(out_size, frame1.ConvertToRgbBuffer(cricket::FOURCC_R444,
                                                 out,
                                                 out_size, kWidth * 2));
    EXPECT_TRUE(LoadFrame(out, out_size, cricket::FOURCC_R444,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 20));
  }

// Macro to help test different rotations
#define TEST_MIRROR(FOURCC, BPP)                                               \
  void Construct##FOURCC##Mirror() {                                           \
    T frame1, frame2, frame3;                                                  \
    std::unique_ptr<rtc::MemoryStream> ms(                                     \
        CreateYuvSample(kWidth, kHeight, BPP));                                \
    ASSERT_TRUE(ms.get() != NULL);                                             \
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_##FOURCC, kWidth,          \
                          -kHeight, kWidth, kHeight,                           \
                          webrtc::kVideoRotation_180, &frame1));               \
    size_t data_size;                                                          \
    bool ret = ms->GetSize(&data_size);                                        \
    EXPECT_TRUE(ret);                                                          \
    EXPECT_TRUE(frame2.Init(cricket::FOURCC_##FOURCC, kWidth, kHeight, kWidth, \
                            kHeight,                                           \
                            reinterpret_cast<uint8_t*>(ms->GetBuffer()),       \
                            data_size, 0, webrtc::kVideoRotation_0));          \
    int width_rotate = frame1.width();                                         \
    int height_rotate = frame1.height();                                       \
    frame3.InitToEmptyBuffer(width_rotate, height_rotate, 0);                  \
    libyuv::I420Mirror(frame2.video_frame_buffer()->DataY(),                   \
                       frame2.video_frame_buffer()->StrideY(),                 \
                       frame2.video_frame_buffer()->DataU(),                   \
                       frame2.video_frame_buffer()->StrideU(),                 \
                       frame2.video_frame_buffer()->DataV(),                   \
                       frame2.video_frame_buffer()->StrideV(),                 \
                       frame3.video_frame_buffer()->MutableDataY(),            \
                       frame3.video_frame_buffer()->StrideY(),                 \
                       frame3.video_frame_buffer()->MutableDataU(),            \
                       frame3.video_frame_buffer()->StrideU(),                 \
                       frame3.video_frame_buffer()->MutableDataV(),            \
                       frame3.video_frame_buffer()->StrideV(),                 \
                       kWidth, kHeight);                                       \
    EXPECT_TRUE(IsEqual(frame1, frame3, 0));                                   \
  }

  TEST_MIRROR(I420, 420)

// Macro to help test different rotations
#define TEST_ROTATE(FOURCC, BPP, ROTATE)                                       \
  void Construct##FOURCC##Rotate##ROTATE() {                                   \
    T frame1, frame2, frame3;                                                  \
    std::unique_ptr<rtc::MemoryStream> ms(                                     \
        CreateYuvSample(kWidth, kHeight, BPP));                                \
    ASSERT_TRUE(ms.get() != NULL);                                             \
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_##FOURCC, kWidth, kHeight, \
                          kWidth, kHeight, webrtc::kVideoRotation_##ROTATE,    \
                          &frame1));                                           \
    size_t data_size;                                                          \
    bool ret = ms->GetSize(&data_size);                                        \
    EXPECT_TRUE(ret);                                                          \
    EXPECT_TRUE(frame2.Init(cricket::FOURCC_##FOURCC, kWidth, kHeight, kWidth, \
                            kHeight,                                           \
                            reinterpret_cast<uint8_t*>(ms->GetBuffer()),       \
                            data_size, 0, webrtc::kVideoRotation_0));          \
    int width_rotate = frame1.width();                                         \
    int height_rotate = frame1.height();                                       \
    frame3.InitToEmptyBuffer(width_rotate, height_rotate, 0);                  \
    libyuv::I420Rotate(frame2.video_frame_buffer()->DataY(),                   \
                       frame2.video_frame_buffer()->StrideY(),                 \
                       frame2.video_frame_buffer()->DataU(),                   \
                       frame2.video_frame_buffer()->StrideU(),                 \
                       frame2.video_frame_buffer()->DataV(),                   \
                       frame2.video_frame_buffer()->StrideV(),                 \
                       frame3.video_frame_buffer()->MutableDataY(),            \
                       frame3.video_frame_buffer()->StrideY(),                 \
                       frame3.video_frame_buffer()->MutableDataU(),            \
                       frame3.video_frame_buffer()->StrideU(),                 \
                       frame3.video_frame_buffer()->MutableDataV(),            \
                       frame3.video_frame_buffer()->StrideV(),                 \
                       kWidth, kHeight, libyuv::kRotate##ROTATE);              \
    EXPECT_TRUE(IsEqual(frame1, frame3, 0));                                   \
  }

  // Test constructing an image with rotation.
  TEST_ROTATE(I420, 12, 0)
  TEST_ROTATE(I420, 12, 90)
  TEST_ROTATE(I420, 12, 180)
  TEST_ROTATE(I420, 12, 270)
  TEST_ROTATE(YV12, 12, 0)
  TEST_ROTATE(YV12, 12, 90)
  TEST_ROTATE(YV12, 12, 180)
  TEST_ROTATE(YV12, 12, 270)
  TEST_ROTATE(NV12, 12, 0)
  TEST_ROTATE(NV12, 12, 90)
  TEST_ROTATE(NV12, 12, 180)
  TEST_ROTATE(NV12, 12, 270)
  TEST_ROTATE(NV21, 12, 0)
  TEST_ROTATE(NV21, 12, 90)
  TEST_ROTATE(NV21, 12, 180)
  TEST_ROTATE(NV21, 12, 270)
  TEST_ROTATE(UYVY, 16, 0)
  TEST_ROTATE(UYVY, 16, 90)
  TEST_ROTATE(UYVY, 16, 180)
  TEST_ROTATE(UYVY, 16, 270)
  TEST_ROTATE(YUY2, 16, 0)
  TEST_ROTATE(YUY2, 16, 90)
  TEST_ROTATE(YUY2, 16, 180)
  TEST_ROTATE(YUY2, 16, 270)

  // Test constructing an image from a UYVY buffer rotated 90 degrees.
  void ConstructUyvyRotate90() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_UYVY, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_UYVY, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_90, &frame2));
  }

  // Test constructing an image from a UYVY buffer rotated 180 degrees.
  void ConstructUyvyRotate180() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_UYVY, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_UYVY, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_180,
                          &frame2));
  }

  // Test constructing an image from a UYVY buffer rotated 270 degrees.
  void ConstructUyvyRotate270() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_UYVY, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_UYVY, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_270,
                          &frame2));
  }

  // Test constructing an image from a YUY2 buffer rotated 90 degrees.
  void ConstructYuy2Rotate90() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_90, &frame2));
  }

  // Test constructing an image from a YUY2 buffer rotated 180 degrees.
  void ConstructYuy2Rotate180() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_180,
                          &frame2));
  }

  // Test constructing an image from a YUY2 buffer rotated 270 degrees.
  void ConstructYuy2Rotate270() {
    T frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                          kWidth, kHeight, webrtc::kVideoRotation_270,
                          &frame2));
  }

  // Test 1 pixel edge case image I420 buffer.
  void ConstructI4201Pixel() {
    T frame;
    uint8_t pixel[3] = {1, 2, 3};
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame.Init(cricket::FOURCC_I420, 1, 1, 1, 1, pixel,
                             sizeof(pixel), 0, webrtc::kVideoRotation_0));
    }
    const uint8_t* y = pixel;
    const uint8_t* u = y + 1;
    const uint8_t* v = u + 1;
    EXPECT_TRUE(IsEqual(frame, 1, 1, 0, y, 1, u, 1, v, 1, 0));
  }

  // Test 5 pixel edge case image.
  void ConstructI4205Pixel() {
    T frame;
    uint8_t pixels5x5[5 * 5 + ((5 + 1) / 2 * (5 + 1) / 2) * 2];
    memset(pixels5x5, 1, 5 * 5 + ((5 + 1) / 2 * (5 + 1) / 2) *  2);
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame.Init(cricket::FOURCC_I420, 5, 5, 5, 5, pixels5x5,
                             sizeof(pixels5x5), 0,
                             webrtc::kVideoRotation_0));
    }
    EXPECT_EQ(5, frame.width());
    EXPECT_EQ(5, frame.height());
    EXPECT_EQ(5, frame.video_frame_buffer()->StrideY());
    EXPECT_EQ(3, frame.video_frame_buffer()->StrideU());
    EXPECT_EQ(3, frame.video_frame_buffer()->StrideV());
  }

  // Test 1 pixel edge case image ARGB buffer.
  void ConstructARGB1Pixel() {
    T frame;
    uint8_t pixel[4] = {64, 128, 192, 255};
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame.Init(cricket::FOURCC_ARGB, 1, 1, 1, 1, pixel,
                             sizeof(pixel), 0,
                             webrtc::kVideoRotation_0));
    }
    // Convert back to ARGB.
    size_t out_size = 4;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment]);
    uint8_t* out = ALIGNP(outbuf.get(), kAlignment);

    EXPECT_EQ(out_size, frame.ConvertToRgbBuffer(cricket::FOURCC_ARGB,
                                                 out,
                                                 out_size,    // buffer size
                                                 out_size));  // stride
  #ifdef USE_LMI_CONVERT
    // TODO(fbarchard): Expected to fail, but not crash.
    EXPECT_FALSE(IsPlaneEqual("argb", pixel, 4, out, 4, 3, 1, 2));
  #else
    // TODO(fbarchard): Check for overwrite.
    EXPECT_TRUE(IsPlaneEqual("argb", pixel, 4, out, 4, 3, 1, 2));
  #endif
  }

  // Test Black, White and Grey pixels.
  void ConstructARGBBlackWhitePixel() {
    T frame;
    uint8_t pixel[10 * 4] = {0,   0,   0,   255,   // Black.
                             0,   0,   0,   255,   // Black.
                             64,  64,  64,  255,   // Dark Grey.
                             64,  64,  64,  255,   // Dark Grey.
                             128, 128, 128, 255,   // Grey.
                             128, 128, 128, 255,   // Grey.
                             196, 196, 196, 255,   // Light Grey.
                             196, 196, 196, 255,   // Light Grey.
                             255, 255, 255, 255,   // White.
                             255, 255, 255, 255};  // White.

    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame.Init(cricket::FOURCC_ARGB, 10, 1, 10, 1, pixel,
                             sizeof(pixel), 1, 1, 0,
                             webrtc::kVideoRotation_0));
    }
    // Convert back to ARGB
    size_t out_size = 10 * 4;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment]);
    uint8_t* out = ALIGNP(outbuf.get(), kAlignment);

    EXPECT_EQ(out_size, frame.ConvertToRgbBuffer(cricket::FOURCC_ARGB,
                                                 out,
                                                 out_size,    // buffer size.
                                                 out_size));  // stride.
    EXPECT_TRUE(IsPlaneEqual("argb", pixel, out_size,
                             out, out_size,
                             out_size, 1, 2));
  }

  // Test constructing an image from an I420 buffer with horizontal cropping.
  void ConstructI420CropHorizontal() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(kImageFilename, kYuvExtension,
                          cricket::FOURCC_I420, kWidth, kHeight,
                          kWidth * 3 / 4, kHeight, webrtc::kVideoRotation_0,
                          &frame2));
    EXPECT_TRUE(IsEqualWithCrop(frame2, frame1, kWidth / 8, 0, 0));
  }

  // Test constructing an image from a YUY2 buffer with horizontal cropping.
  void ConstructYuy2CropHorizontal() {
    T frame1, frame2;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(ConvertYuv422(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                              &frame1));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                          kWidth * 3 / 4, kHeight, webrtc::kVideoRotation_0,
                          &frame2));
    EXPECT_TRUE(IsEqualWithCrop(frame2, frame1, kWidth / 8, 0, 0));
  }

  // Test constructing an image from an ARGB buffer with horizontal cropping.
  void ConstructARGBCropHorizontal() {
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_ARGB, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ARGB,
                                           kWidth, kHeight);
    ASSERT_TRUE(frame1);
    T frame2;
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ARGB, kWidth, kHeight,
                          kWidth * 3 / 4, kHeight, webrtc::kVideoRotation_0,
                          &frame2));
    EXPECT_TRUE(IsEqualWithCrop(frame2, *frame1, kWidth / 8, 0, 2));
  }

  // Test constructing an image from an I420 buffer, cropping top and bottom.
  void ConstructI420CropVertical() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(LoadSample(kImageFilename, kYuvExtension).get(),
                          cricket::FOURCC_I420, kWidth, kHeight,
                          kWidth, kHeight * 3 / 4, webrtc::kVideoRotation_0,
                          &frame2));
    EXPECT_TRUE(IsEqualWithCrop(frame2, frame1, 0, kHeight / 8, 0));
  }

  // Test constructing an image from I420 synonymous formats.
  void ConstructI420Aliases() {
    T frame1, frame2, frame3;
    ASSERT_TRUE(LoadFrame(LoadSample(kImageFilename, kYuvExtension),
                          cricket::FOURCC_I420, kWidth, kHeight,
                          &frame1));
    ASSERT_TRUE(LoadFrame(kImageFilename, kYuvExtension,
                          cricket::FOURCC_IYUV, kWidth, kHeight,
                          &frame2));
    ASSERT_TRUE(LoadFrame(kImageFilename, kYuvExtension,
                          cricket::FOURCC_YU12, kWidth, kHeight,
                          &frame3));
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
    EXPECT_TRUE(IsEqual(frame1, frame3, 0));
  }

  // Test constructing an image from an I420 MJPG buffer.
  void ConstructMjpgI420() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(kJpeg420Filename, kJpegExtension,
                          cricket::FOURCC_MJPG, kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 32));
  }

  // Test constructing an image from an I422 MJPG buffer.
  void ConstructMjpgI422() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(LoadSample(kJpeg422Filename, kJpegExtension).get(),
                          cricket::FOURCC_MJPG, kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 32));
  }

  // Test constructing an image from an I444 MJPG buffer.
  void ConstructMjpgI444() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(LoadSample(kJpeg444Filename, kJpegExtension),
                          cricket::FOURCC_MJPG, kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 32));
  }

  // Test constructing an image from an I444 MJPG buffer.
  void ConstructMjpgI411() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(kJpeg411Filename, kJpegExtension,
                          cricket::FOURCC_MJPG, kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsEqual(frame1, frame2, 32));
  }

  // Test constructing an image from an I400 MJPG buffer.
  // TODO(fbarchard): Stronger compare on chroma.  Compare agaisnt a grey image.
  void ConstructMjpgI400() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    ASSERT_TRUE(LoadFrame(kJpeg400Filename, kJpegExtension,
                          cricket::FOURCC_MJPG, kWidth, kHeight, &frame2));
    EXPECT_TRUE(IsPlaneEqual("y", frame1.video_frame_buffer()->DataY(),
                             frame1.video_frame_buffer()->StrideY(),
                             frame2.video_frame_buffer()->DataY(),
                             frame2.video_frame_buffer()->StrideY(),
                             kWidth, kHeight, 32));
    EXPECT_TRUE(IsEqual(frame1, frame2, 128));
  }

  // Test constructing an image from an I420 MJPG buffer.
  void ValidateFrame(const char* name,
                     const char* extension,
                     uint32_t fourcc,
                     int data_adjust,
                     int size_adjust,
                     bool expected_result) {
    T frame;
    std::unique_ptr<rtc::MemoryStream> ms(LoadSample(name, extension));
    ASSERT_TRUE(ms.get() != NULL);
    const uint8_t* sample =
        reinterpret_cast<const uint8_t*>(ms.get()->GetBuffer());
    size_t sample_size;
    ms->GetSize(&sample_size);
    // Optional adjust size to test invalid size.
    size_t data_size = sample_size + data_adjust;

    // Allocate a buffer with end page aligned.
    const int kPadToHeapSized = 16 * 1024 * 1024;
    std::unique_ptr<uint8_t[]> page_buffer(
        new uint8_t[((data_size + kPadToHeapSized + 4095) & ~4095)]);
    uint8_t* data_ptr = page_buffer.get();
    if (!data_ptr) {
      LOG(LS_WARNING) << "Failed to allocate memory for ValidateFrame test.";
      EXPECT_FALSE(expected_result);  // NULL is okay if failure was expected.
      return;
    }
    data_ptr += kPadToHeapSized + (-(static_cast<int>(data_size)) & 4095);
    memcpy(data_ptr, sample, std::min(data_size, sample_size));
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_EQ(expected_result, frame.Validate(fourcc, kWidth, kHeight,
                                                data_ptr,
                                                sample_size + size_adjust));
    }
  }

  // Test validate for I420 MJPG buffer.
  void ValidateMjpgI420() {
    ValidateFrame(kJpeg420Filename, kJpegExtension,
                  cricket::FOURCC_MJPG, 0, 0, true);
  }

  // Test validate for I422 MJPG buffer.
  void ValidateMjpgI422() {
    ValidateFrame(kJpeg422Filename, kJpegExtension,
                  cricket::FOURCC_MJPG, 0, 0, true);
  }

  // Test validate for I444 MJPG buffer.
  void ValidateMjpgI444() {
    ValidateFrame(kJpeg444Filename, kJpegExtension,
                  cricket::FOURCC_MJPG, 0, 0, true);
  }

  // Test validate for I411 MJPG buffer.
  void ValidateMjpgI411() {
    ValidateFrame(kJpeg411Filename, kJpegExtension,
                  cricket::FOURCC_MJPG, 0, 0, true);
  }

  // Test validate for I400 MJPG buffer.
  void ValidateMjpgI400() {
    ValidateFrame(kJpeg400Filename, kJpegExtension,
                  cricket::FOURCC_MJPG, 0, 0, true);
  }

  // Test validate for I420 buffer.
  void ValidateI420() {
    ValidateFrame(kImageFilename, kYuvExtension,
                  cricket::FOURCC_I420, 0, 0, true);
  }

  // Test validate for I420 buffer where size is too small
  void ValidateI420SmallSize() {
    ValidateFrame(kImageFilename, kYuvExtension,
                  cricket::FOURCC_I420, 0, -16384, false);
  }

  // Test validate for I420 buffer where size is too large (16 MB)
  // Will produce warning but pass.
  void ValidateI420LargeSize() {
    ValidateFrame(kImageFilename, kYuvExtension,
                  cricket::FOURCC_I420, 16000000, 16000000,
                  true);
  }

  // Test validate for I420 buffer where size is 1 GB (not reasonable).
  void ValidateI420HugeSize() {
#ifndef WIN32  // TODO(fbarchard): Reenable when fixing bug 9603762.
    ValidateFrame(kImageFilename, kYuvExtension,
                  cricket::FOURCC_I420, 1000000000u,
                  1000000000u, false);
#endif
  }

  // The following test that Validate crashes if the size is greater than the
  // actual buffer size.
  // TODO(fbarchard): Consider moving a filter into the capturer/plugin.
#if defined(_MSC_VER) && !defined(NDEBUG)
  int ExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep) {
    if (code == EXCEPTION_ACCESS_VIOLATION) {
      LOG(LS_INFO) << "Caught EXCEPTION_ACCESS_VIOLATION as expected.";
      return EXCEPTION_EXECUTE_HANDLER;
    } else {
      LOG(LS_INFO) << "Did not catch EXCEPTION_ACCESS_VIOLATION.  Unexpected.";
      return EXCEPTION_CONTINUE_SEARCH;
    }
  }

  // Test validate fails for truncated MJPG data buffer.  If ValidateFrame
  // crashes the exception handler will return and unittest passes with OK.
  void ValidateMjpgI420InvalidSize() {
    __try {
      ValidateFrame(kJpeg420Filename,  kJpegExtension,
                    cricket::FOURCC_MJPG, -16384, 0, false);
      FAIL() << "Validate was expected to cause EXCEPTION_ACCESS_VIOLATION.";
    } __except(ExceptionFilter(GetExceptionCode(), GetExceptionInformation())) {
      return;  // Successfully crashed in ValidateFrame.
    }
  }

  // Test validate fails for truncated I420 buffer.
  void ValidateI420InvalidSize() {
    __try {
      ValidateFrame(kImageFilename, kYuvExtension,
                    cricket::FOURCC_I420, -16384, 0, false);
      FAIL() << "Validate was expected to cause EXCEPTION_ACCESS_VIOLATION.";
    } __except(ExceptionFilter(GetExceptionCode(), GetExceptionInformation())) {
      return;  // Successfully crashed in ValidateFrame.
    }
  }
#endif

  // Test constructing an image from a YUY2 buffer (and synonymous formats).
  void ConstructYuy2Aliases() {
    T frame1, frame2, frame3, frame4;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_YUY2, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(ConvertYuv422(ms.get(), cricket::FOURCC_YUY2, kWidth, kHeight,
                              &frame1));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUVS,
                          kWidth, kHeight, &frame3));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUYV,
                          kWidth, kHeight, &frame4));
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
    EXPECT_TRUE(IsEqual(frame1, frame3, 0));
    EXPECT_TRUE(IsEqual(frame1, frame4, 0));
  }

  // Test constructing an image from a UYVY buffer (and synonymous formats).
  void ConstructUyvyAliases() {
    T frame1, frame2, frame3, frame4;
    std::unique_ptr<rtc::MemoryStream> ms(
        CreateYuv422Sample(cricket::FOURCC_UYVY, kWidth, kHeight));
    ASSERT_TRUE(ms.get() != NULL);
    EXPECT_TRUE(ConvertYuv422(ms.get(), cricket::FOURCC_UYVY, kWidth, kHeight,
                              &frame1));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_UYVY,
                          kWidth, kHeight, &frame2));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_2VUY,
                          kWidth, kHeight, &frame3));
    EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_HDYC,
                          kWidth, kHeight, &frame4));
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
    EXPECT_TRUE(IsEqual(frame1, frame3, 0));
    EXPECT_TRUE(IsEqual(frame1, frame4, 0));
  }

  // Test creating a copy.
  void ConstructCopy() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame2.Init(frame1));
    }
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
  }

  // Test creating a copy and check that it just increments the refcount.
  void ConstructCopyIsRef() {
    T frame1, frame2;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_TRUE(frame2.Init(frame1));
    }
    EXPECT_TRUE(IsEqual(frame1, frame2, 0));
    EXPECT_EQ(frame1.video_frame_buffer(), frame2.video_frame_buffer());
  }

  // Test constructing an image from a YUY2 buffer with a range of sizes.
  // Only tests that conversion does not crash or corrupt heap.
  void ConstructYuy2AllSizes() {
    T frame1, frame2;
    for (int height = kMinHeightAll; height <= kMaxHeightAll; ++height) {
      for (int width = kMinWidthAll; width <= kMaxWidthAll; ++width) {
        std::unique_ptr<rtc::MemoryStream> ms(
            CreateYuv422Sample(cricket::FOURCC_YUY2, width, height));
        ASSERT_TRUE(ms.get() != NULL);
        EXPECT_TRUE(ConvertYuv422(ms.get(), cricket::FOURCC_YUY2, width, height,
                                  &frame1));
        EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_YUY2,
                              width, height, &frame2));
        EXPECT_TRUE(IsEqual(frame1, frame2, 0));
      }
    }
  }

  // Test constructing an image from a ARGB buffer with a range of sizes.
  // Only tests that conversion does not crash or corrupt heap.
  void ConstructARGBAllSizes() {
    for (int height = kMinHeightAll; height <= kMaxHeightAll; ++height) {
      for (int width = kMinWidthAll; width <= kMaxWidthAll; ++width) {
        std::unique_ptr<rtc::MemoryStream> ms(
            CreateRgbSample(cricket::FOURCC_ARGB, width, height));
        ASSERT_TRUE(ms.get() != NULL);
        std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ARGB,
                                               width, height);
        ASSERT_TRUE(frame1);
        T frame2;
        EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ARGB,
                              width, height, &frame2));
        EXPECT_TRUE(IsEqual(*frame1, frame2, 64));
      }
    }
    // Test a practical window size for screencasting usecase.
    const int kOddWidth = 1228;
    const int kOddHeight = 260;
    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < 2; ++i) {
        std::unique_ptr<rtc::MemoryStream> ms(
        CreateRgbSample(cricket::FOURCC_ARGB, kOddWidth + i, kOddHeight + j));
        ASSERT_TRUE(ms.get() != NULL);
        std::unique_ptr<T> frame1 = ConvertRgb(ms.get(), cricket::FOURCC_ARGB,
                                               kOddWidth + i, kOddHeight + j);
        ASSERT_TRUE(frame1);
        T frame2;
        EXPECT_TRUE(LoadFrame(ms.get(), cricket::FOURCC_ARGB,
                              kOddWidth + i, kOddHeight + j, &frame2));
        EXPECT_TRUE(IsEqual(*frame1, frame2, 64));
      }
    }
  }

  //////////////////////
  // Conversion tests //
  //////////////////////

  enum ToFrom { TO, FROM };

  // Helper function for test converting from I420 to packed formats.
  inline void ConvertToBuffer(int bpp,
                              int rowpad,
                              bool invert,
                              ToFrom to_from,
                              int error,
                              uint32_t fourcc,
                              int (*RGBToI420)(const uint8_t* src_frame,
                                               int src_stride_frame,
                                               uint8_t* dst_y,
                                               int dst_stride_y,
                                               uint8_t* dst_u,
                                               int dst_stride_u,
                                               uint8_t* dst_v,
                                               int dst_stride_v,
                                               int width,
                                               int height)) {
    T frame1, frame2;
    int repeat_to = (to_from == TO) ? repeat_ : 1;
    int repeat_from  = (to_from == FROM) ? repeat_ : 1;

    int astride = kWidth * bpp + rowpad;
    size_t out_size = astride * kHeight;
    std::unique_ptr<uint8_t[]> outbuf(new uint8_t[out_size + kAlignment + 1]);
    memset(outbuf.get(), 0, out_size + kAlignment + 1);
    uint8_t* outtop = ALIGNP(outbuf.get(), kAlignment);
    uint8_t* out = outtop;
    int stride = astride;
    if (invert) {
      out += (kHeight - 1) * stride;  // Point to last row.
      stride = -stride;
    }
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));

    for (int i = 0; i < repeat_to; ++i) {
      EXPECT_EQ(out_size, frame1.ConvertToRgbBuffer(fourcc,
                                                    out,
                                                    out_size, stride));
    }
    frame2.InitToEmptyBuffer(kWidth, kHeight, 0);
    for (int i = 0; i < repeat_from; ++i) {
      EXPECT_EQ(0, RGBToI420(out, stride,
                             frame2.video_frame_buffer()->MutableDataY(),
                             frame2.video_frame_buffer()->StrideY(),
                             frame2.video_frame_buffer()->MutableDataU(),
                             frame2.video_frame_buffer()->StrideU(),
                             frame2.video_frame_buffer()->MutableDataV(),
                             frame2.video_frame_buffer()->StrideV(),
                             kWidth, kHeight));
    }
    if (rowpad) {
      EXPECT_EQ(0, outtop[kWidth * bpp]);  // Ensure stride skipped end of row.
      EXPECT_NE(0, outtop[astride]);       // Ensure pixel at start of 2nd row.
    } else {
      EXPECT_NE(0, outtop[kWidth * bpp]);  // Expect something to be here.
    }
    EXPECT_EQ(0, outtop[out_size]);      // Ensure no overrun.
    EXPECT_TRUE(IsEqual(frame1, frame2, error));
  }

  static const int kError = 20;
  static const int kErrorHigh = 40;
  static const int kOddStride = 23;

  // Tests ConvertToRGBBuffer formats.
  void ConvertToARGBBuffer() {
    ConvertToBuffer(4, 0, false, TO, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertToBGRABuffer() {
    ConvertToBuffer(4, 0, false, TO, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertToABGRBuffer() {
    ConvertToBuffer(4, 0, false, TO, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertToRGB24Buffer() {
    ConvertToBuffer(3, 0, false, TO, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertToRAWBuffer() {
    ConvertToBuffer(3, 0, false, TO, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertToRGB565Buffer() {
    ConvertToBuffer(2, 0, false, TO, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertToARGB1555Buffer() {
    ConvertToBuffer(2, 0, false, TO, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertToARGB4444Buffer() {
    ConvertToBuffer(2, 0, false, TO, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertToI400Buffer() {
    ConvertToBuffer(1, 0, false, TO, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertToYUY2Buffer() {
    ConvertToBuffer(2, 0, false, TO, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertToUYVYBuffer() {
    ConvertToBuffer(2, 0, false, TO, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Tests ConvertToRGBBuffer formats with odd stride.
  void ConvertToARGBBufferStride() {
    ConvertToBuffer(4, kOddStride, false, TO, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertToBGRABufferStride() {
    ConvertToBuffer(4, kOddStride, false, TO, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertToABGRBufferStride() {
    ConvertToBuffer(4, kOddStride, false, TO, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertToRGB24BufferStride() {
    ConvertToBuffer(3, kOddStride, false, TO, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertToRAWBufferStride() {
    ConvertToBuffer(3, kOddStride, false, TO, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertToRGB565BufferStride() {
    ConvertToBuffer(2, kOddStride, false, TO, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertToARGB1555BufferStride() {
    ConvertToBuffer(2, kOddStride, false, TO, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertToARGB4444BufferStride() {
    ConvertToBuffer(2, kOddStride, false, TO, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertToI400BufferStride() {
    ConvertToBuffer(1, kOddStride, false, TO, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertToYUY2BufferStride() {
    ConvertToBuffer(2, kOddStride, false, TO, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertToUYVYBufferStride() {
    ConvertToBuffer(2, kOddStride, false, TO, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Tests ConvertToRGBBuffer formats with negative stride to invert image.
  void ConvertToARGBBufferInverted() {
    ConvertToBuffer(4, 0, true, TO, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertToBGRABufferInverted() {
    ConvertToBuffer(4, 0, true, TO, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertToABGRBufferInverted() {
    ConvertToBuffer(4, 0, true, TO, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertToRGB24BufferInverted() {
    ConvertToBuffer(3, 0, true, TO, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertToRAWBufferInverted() {
    ConvertToBuffer(3, 0, true, TO, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertToRGB565BufferInverted() {
    ConvertToBuffer(2, 0, true, TO, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertToARGB1555BufferInverted() {
    ConvertToBuffer(2, 0, true, TO, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertToARGB4444BufferInverted() {
    ConvertToBuffer(2, 0, true, TO, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertToI400BufferInverted() {
    ConvertToBuffer(1, 0, true, TO, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertToYUY2BufferInverted() {
    ConvertToBuffer(2, 0, true, TO, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertToUYVYBufferInverted() {
    ConvertToBuffer(2, 0, true, TO, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Tests ConvertFrom formats.
  void ConvertFromARGBBuffer() {
    ConvertToBuffer(4, 0, false, FROM, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertFromBGRABuffer() {
    ConvertToBuffer(4, 0, false, FROM, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertFromABGRBuffer() {
    ConvertToBuffer(4, 0, false, FROM, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertFromRGB24Buffer() {
    ConvertToBuffer(3, 0, false, FROM, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertFromRAWBuffer() {
    ConvertToBuffer(3, 0, false, FROM, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertFromRGB565Buffer() {
    ConvertToBuffer(2, 0, false, FROM, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertFromARGB1555Buffer() {
    ConvertToBuffer(2, 0, false, FROM, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertFromARGB4444Buffer() {
    ConvertToBuffer(2, 0, false, FROM, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertFromI400Buffer() {
    ConvertToBuffer(1, 0, false, FROM, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertFromYUY2Buffer() {
    ConvertToBuffer(2, 0, false, FROM, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertFromUYVYBuffer() {
    ConvertToBuffer(2, 0, false, FROM, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Tests ConvertFrom formats with odd stride.
  void ConvertFromARGBBufferStride() {
    ConvertToBuffer(4, kOddStride, false, FROM, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertFromBGRABufferStride() {
    ConvertToBuffer(4, kOddStride, false, FROM, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertFromABGRBufferStride() {
    ConvertToBuffer(4, kOddStride, false, FROM, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertFromRGB24BufferStride() {
    ConvertToBuffer(3, kOddStride, false, FROM, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertFromRAWBufferStride() {
    ConvertToBuffer(3, kOddStride, false, FROM, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertFromRGB565BufferStride() {
    ConvertToBuffer(2, kOddStride, false, FROM, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertFromARGB1555BufferStride() {
    ConvertToBuffer(2, kOddStride, false, FROM, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertFromARGB4444BufferStride() {
    ConvertToBuffer(2, kOddStride, false, FROM, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertFromI400BufferStride() {
    ConvertToBuffer(1, kOddStride, false, FROM, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertFromYUY2BufferStride() {
    ConvertToBuffer(2, kOddStride, false, FROM, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertFromUYVYBufferStride() {
    ConvertToBuffer(2, kOddStride, false, FROM, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Tests ConvertFrom formats with negative stride to invert image.
  void ConvertFromARGBBufferInverted() {
    ConvertToBuffer(4, 0, true, FROM, kError,
                    cricket::FOURCC_ARGB, libyuv::ARGBToI420);
  }
  void ConvertFromBGRABufferInverted() {
    ConvertToBuffer(4, 0, true, FROM, kError,
                    cricket::FOURCC_BGRA, libyuv::BGRAToI420);
  }
  void ConvertFromABGRBufferInverted() {
    ConvertToBuffer(4, 0, true, FROM, kError,
                    cricket::FOURCC_ABGR, libyuv::ABGRToI420);
  }
  void ConvertFromRGB24BufferInverted() {
    ConvertToBuffer(3, 0, true, FROM, kError,
                    cricket::FOURCC_24BG, libyuv::RGB24ToI420);
  }
  void ConvertFromRAWBufferInverted() {
    ConvertToBuffer(3, 0, true, FROM, kError,
                    cricket::FOURCC_RAW, libyuv::RAWToI420);
  }
  void ConvertFromRGB565BufferInverted() {
    ConvertToBuffer(2, 0, true, FROM, kError,
                    cricket::FOURCC_RGBP, libyuv::RGB565ToI420);
  }
  void ConvertFromARGB1555BufferInverted() {
    ConvertToBuffer(2, 0, true, FROM, kError,
                    cricket::FOURCC_RGBO, libyuv::ARGB1555ToI420);
  }
  void ConvertFromARGB4444BufferInverted() {
    ConvertToBuffer(2, 0, true, FROM, kError,
                    cricket::FOURCC_R444, libyuv::ARGB4444ToI420);
  }
  void ConvertFromI400BufferInverted() {
    ConvertToBuffer(1, 0, true, FROM, 128,
                    cricket::FOURCC_I400, libyuv::I400ToI420);
  }
  void ConvertFromYUY2BufferInverted() {
    ConvertToBuffer(2, 0, true, FROM, kError,
                    cricket::FOURCC_YUY2, libyuv::YUY2ToI420);
  }
  void ConvertFromUYVYBufferInverted() {
    ConvertToBuffer(2, 0, true, FROM, kError,
                    cricket::FOURCC_UYVY, libyuv::UYVYToI420);
  }

  // Test converting from I420 to I422.
  void ConvertToI422Buffer() {
    T frame1, frame2;
    size_t out_size = kWidth * kHeight * 2;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[out_size + kAlignment]);
    uint8_t* y = ALIGNP(buf.get(), kAlignment);
    uint8_t* u = y + kWidth * kHeight;
    uint8_t* v = u + (kWidth / 2) * kHeight;
    ASSERT_TRUE(LoadFrameNoRepeat(&frame1));
    for (int i = 0; i < repeat_; ++i) {
      EXPECT_EQ(0, libyuv::I420ToI422(frame1.video_frame_buffer()->DataY(),
                                      frame1.video_frame_buffer()->StrideY(),
                                      frame1.video_frame_buffer()->DataU(),
                                      frame1.video_frame_buffer()->StrideU(),
                                      frame1.video_frame_buffer()->DataV(),
                                      frame1.video_frame_buffer()->StrideV(),
                                      y, kWidth,
                                      u, kWidth / 2,
                                      v, kWidth / 2,
                                      kWidth, kHeight));
    }
    EXPECT_TRUE(frame2.Init(cricket::FOURCC_I422, kWidth, kHeight, kWidth,
                            kHeight, y, out_size, 1, 1, 0,
                            webrtc::kVideoRotation_0));
    EXPECT_TRUE(IsEqual(frame1, frame2, 1));
  }

  ///////////////////
  // General tests //
  ///////////////////

  void Copy() {
    std::unique_ptr<T> source(new T);
    std::unique_ptr<cricket::VideoFrame> target;
    ASSERT_TRUE(LoadFrameNoRepeat(source.get()));
    target.reset(source->Copy());
    EXPECT_TRUE(IsEqual(*source, *target, 0));
    source.reset();
    ASSERT_TRUE(target->video_frame_buffer() != NULL);
    EXPECT_TRUE(target->video_frame_buffer()->DataY() != NULL);
  }

  void CopyIsRef() {
    std::unique_ptr<T> source(new T);
    std::unique_ptr<const cricket::VideoFrame> target;
    ASSERT_TRUE(LoadFrameNoRepeat(source.get()));
    target.reset(source->Copy());
    EXPECT_TRUE(IsEqual(*source, *target, 0));
    const T* const_source = source.get();
    EXPECT_EQ(const_source->video_frame_buffer(), target->video_frame_buffer());
  }

  int repeat_;
};

#endif  // WEBRTC_MEDIA_BASE_VIDEOFRAME_UNITTEST_H_
