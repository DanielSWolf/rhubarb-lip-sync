/*
 *  Copyright (c) 2010 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
// A factory to create a GUI video renderer.

#ifndef WEBRTC_MEDIA_DEVICES_VIDEORENDERERFACTORY_H_
#define WEBRTC_MEDIA_DEVICES_VIDEORENDERERFACTORY_H_

#include "webrtc/media/base/videosinkinterface.h"
#if defined(WEBRTC_LINUX) && defined(HAVE_GTK)
#include "webrtc/media/devices/gtkvideorenderer.h"
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS) && !defined(CARBON_DEPRECATED)
#include "webrtc/media/devices/carbonvideorenderer.h"
#elif defined(WIN32)
#include "webrtc/media/devices/gdivideorenderer.h"
#endif

namespace cricket {

class VideoRendererFactory {
 public:
  static rtc::VideoSinkInterface<cricket::VideoFrame>* CreateGuiVideoRenderer(
      int x,
      int y) {
  #if defined(WEBRTC_LINUX) && defined(HAVE_GTK)
    return new GtkVideoRenderer(x, y);
  #elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS) && \
      !defined(CARBON_DEPRECATED)
    CarbonVideoRenderer* renderer = new CarbonVideoRenderer(x, y);
    // Needs to be initialized on the main thread.
    if (renderer->Initialize()) {
      return renderer;
    } else {
      delete renderer;
      return NULL;
    }
  #elif defined(WIN32)
    return new GdiVideoRenderer(x, y);
  #else
    return NULL;
  #endif
  }
};

}  // namespace cricket

#endif  // WEBRTC_MEDIA_DEVICES_VIDEORENDERERFACTORY_H_
