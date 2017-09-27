/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Implementation of GdiVideoRenderer on Windows

#ifdef WIN32

#include "webrtc/media/devices/gdivideorenderer.h"

#include "webrtc/base/thread.h"
#include "webrtc/base/win32window.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/media/base/videoframe.h"

namespace cricket {

/////////////////////////////////////////////////////////////////////////////
// Definition of private class VideoWindow. We use a worker thread to manage
// the window.
/////////////////////////////////////////////////////////////////////////////
class GdiVideoRenderer::VideoWindow : public rtc::Win32Window {
 public:
  VideoWindow(int x, int y, int width, int height);
  virtual ~VideoWindow();

  // Called when a new frame is available. Upon this call, we send
  // kRenderFrameMsg to the window thread. Context: non-worker thread. It may be
  // better to pass RGB bytes to VideoWindow. However, we pass VideoFrame to put
  // all the thread synchronization within VideoWindow.
  void OnFrame(const VideoFrame& frame);

 protected:
  // Override virtual method of rtc::Win32Window. Context: worker Thread.
  bool OnMessage(UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam,
                 LRESULT& result) override;

 private:
  enum { kSetSizeMsg = WM_USER, kRenderFrameMsg};

  // Called when the video size changes. If it is called the first time, we
  // create and start the thread. Otherwise, we send kSetSizeMsg to the thread.
  // Context: non-worker thread.
  bool SetSize(int width, int height);

  class WindowThread : public rtc::Thread {
   public:
    explicit WindowThread(VideoWindow* window) : window_(window) {}

    virtual ~WindowThread() {
      Stop();
    }

    // Override virtual method of rtc::Thread. Context: worker Thread.
    virtual void Run() {
      // Initialize the window
      if (!window_ || !window_->Initialize()) {
        return;
      }
      // Run the message loop
      MSG msg;
      while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

  private:
    VideoWindow* window_;
  };

  // Context: worker Thread.
  bool Initialize();
  void OnPaint();
  void OnSize(int width, int height, bool frame_changed);
  void OnRenderFrame(const VideoFrame* frame);

  BITMAPINFO bmi_;
  std::unique_ptr<uint8_t[]> image_;
  std::unique_ptr<WindowThread> window_thread_;
  // The initial position of the window.
  int initial_x_;
  int initial_y_;
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of class VideoWindow
/////////////////////////////////////////////////////////////////////////////
GdiVideoRenderer::VideoWindow::VideoWindow(
    int x, int y, int width, int height)
    : initial_x_(x),
      initial_y_(y) {
  memset(&bmi_.bmiHeader, 0, sizeof(bmi_.bmiHeader));
  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi_.bmiHeader.biPlanes = 1;
  bmi_.bmiHeader.biBitCount = 32;
  bmi_.bmiHeader.biCompression = BI_RGB;
  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage = width * height * 4;

  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

GdiVideoRenderer::VideoWindow::~VideoWindow() {
  // Context: caller Thread. We cannot call Destroy() since the window was
  // created by another thread. Instead, we send WM_CLOSE message.
  if (handle()) {
    SendMessage(handle(), WM_CLOSE, 0, 0);
  }
}

bool GdiVideoRenderer::VideoWindow::SetSize(int width, int height) {
  if (!window_thread_.get()) {
    // Create and start the window thread.
    window_thread_.reset(new WindowThread(this));
    return window_thread_->Start();
  } else if (width != bmi_.bmiHeader.biWidth ||
      height != -bmi_.bmiHeader.biHeight) {
    SendMessage(handle(), kSetSizeMsg, 0, MAKELPARAM(width, height));
  }
  return true;
}

void GdiVideoRenderer::VideoWindow::OnFrame(const VideoFrame& video_frame) {
  if (!handle()) {
    return;
  }

  const VideoFrame* frame = video_frame.GetCopyWithRotationApplied();

  if (SetSize(frame->width(), frame->height())) {
    SendMessage(handle(), kRenderFrameMsg, reinterpret_cast<WPARAM>(frame), 0);
  }
}

bool GdiVideoRenderer::VideoWindow::OnMessage(UINT uMsg, WPARAM wParam,
                                              LPARAM lParam, LRESULT& result) {
  switch (uMsg) {
    case WM_PAINT:
      OnPaint();
      return true;

    case WM_DESTROY:
      PostQuitMessage(0);  // post WM_QUIT to end the message loop in Run()
      return false;

    case WM_SIZE:  // The window UI was resized.
      OnSize(LOWORD(lParam), HIWORD(lParam), false);
      return true;

    case kSetSizeMsg:  // The video resolution changed.
      OnSize(LOWORD(lParam), HIWORD(lParam), true);
      return true;

    case kRenderFrameMsg:
      OnRenderFrame(reinterpret_cast<const VideoFrame*>(wParam));
      return true;
  }
  return false;
}

bool GdiVideoRenderer::VideoWindow::Initialize() {
  if (!rtc::Win32Window::Create(
      NULL, L"Video Renderer",
      WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
      WS_EX_APPWINDOW,
      initial_x_, initial_y_,
      bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight)) {
        return false;
  }
  OnSize(bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight, false);
  return true;
}

void GdiVideoRenderer::VideoWindow::OnPaint() {
  RECT rcClient;
  GetClientRect(handle(), &rcClient);
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(handle(), &ps);
  StretchDIBits(hdc,
    0, 0, rcClient.right, rcClient.bottom,  // destination rect
    0, 0, bmi_.bmiHeader.biWidth, -bmi_.bmiHeader.biHeight,  // source rect
    image_.get(), &bmi_, DIB_RGB_COLORS, SRCCOPY);
  EndPaint(handle(), &ps);
}

void GdiVideoRenderer::VideoWindow::OnSize(int width, int height,
                                           bool frame_changed) {
  // Get window and client sizes
  RECT rcClient, rcWindow;
  GetClientRect(handle(), &rcClient);
  GetWindowRect(handle(), &rcWindow);

  // Find offset between window size and client size
  POINT ptDiff;
  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;

  // Resize client
  MoveWindow(handle(), rcWindow.left, rcWindow.top,
             width + ptDiff.x, height + ptDiff.y, false);
  UpdateWindow(handle());
  ShowWindow(handle(), SW_SHOW);

  if (frame_changed && (width != bmi_.bmiHeader.biWidth ||
    height != -bmi_.bmiHeader.biHeight)) {
    // Update the bmi and image buffer
    bmi_.bmiHeader.biWidth = width;
    bmi_.bmiHeader.biHeight = -height;
    bmi_.bmiHeader.biSizeImage = width * height * 4;
    image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
  }
}

void GdiVideoRenderer::VideoWindow::OnRenderFrame(const VideoFrame* frame) {
  if (!frame) {
    return;
  }
  // Convert frame to ARGB format, which is accepted by GDI
  frame->ConvertToRgbBuffer(cricket::FOURCC_ARGB, image_.get(),
                            bmi_.bmiHeader.biSizeImage,
                            bmi_.bmiHeader.biWidth * 4);
  InvalidateRect(handle(), 0, 0);
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of class GdiVideoRenderer
/////////////////////////////////////////////////////////////////////////////
GdiVideoRenderer::GdiVideoRenderer(int x, int y)
    : initial_x_(x),
      initial_y_(y) {
}
GdiVideoRenderer::~GdiVideoRenderer() {}

void GdiVideoRenderer::OnFrame(const VideoFrame& frame) {
  if (!window_.get()) { // Create the window for the first frame
    window_.reset(
        new VideoWindow(initial_x_, initial_y_, frame.width(), frame.height()));
  }
  window_->OnFrame(frame);
}

}  // namespace cricket
#endif  // WIN32
