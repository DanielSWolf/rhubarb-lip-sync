/*
 *  Copyright 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


package org.webrtc;

/**
 * Java version of VideoSourceInterface, extended with stop/restart
 * functionality to allow explicit control of the camera device on android,
 * where there is no support for multiple open capture devices and the cost of
 * holding a camera open (even if MediaStreamTrack.setEnabled(false) is muting
 * its output to the encoder) can be too high to bear.
 */
public class VideoSource extends MediaSource {

  public VideoSource(long nativeSource) {
    super(nativeSource);
  }

  // Stop capture feeding this source.
  public void stop() {
    stop(nativeSource);
  }

  // Restart capture feeding this source.  stop() must have been called since
  // the last call to restart() (if any).  Note that this isn't "start()";
  // sources are started by default at birth.
  public void restart() {
    restart(nativeSource);
  }

  @Override
  public void dispose() {
    super.dispose();
  }

  private static native void stop(long nativeSource);
  private static native void restart(long nativeSource);
}
