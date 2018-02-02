/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import android.test.ActivityTestCase;
import android.test.MoreAsserts;
import android.test.suitebuilder.annotation.SmallTest;

import android.graphics.Point;

import static org.webrtc.RendererCommon.ScalingType.*;
import static org.webrtc.RendererCommon.getDisplaySize;
import static org.webrtc.RendererCommon.getLayoutMatrix;
import static org.webrtc.RendererCommon.rotateTextureMatrix;

public final class RendererCommonTest extends ActivityTestCase {
  @SmallTest
  static public void testDisplaySizeNoFrame() {
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_FIT, 0.0f, 0, 0));
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_FILL, 0.0f, 0, 0));
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_BALANCED, 0.0f, 0, 0));
  }

  @SmallTest
  public static void testDisplaySizeDegenerateAspectRatio() {
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FIT, 0.0f, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FILL, 0.0f, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_BALANCED, 0.0f, 1280, 720));
  }

  @SmallTest
  public static void testZeroDisplaySize() {
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_FIT, 16.0f / 9, 0, 0));
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_FILL, 16.0f / 9, 0, 0));
    assertEquals(new Point(0, 0), getDisplaySize(SCALE_ASPECT_BALANCED, 16.0f / 9, 0, 0));
  }

  @SmallTest
  public static void testDisplaySizePerfectFit() {
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FIT, 16.0f / 9, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FILL, 16.0f / 9, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_BALANCED, 16.0f / 9, 1280, 720));
    assertEquals(new Point(720, 1280), getDisplaySize(SCALE_ASPECT_FIT, 9.0f / 16, 720, 1280));
    assertEquals(new Point(720, 1280), getDisplaySize(SCALE_ASPECT_FILL, 9.0f / 16, 720, 1280));
    assertEquals(new Point(720, 1280), getDisplaySize(SCALE_ASPECT_BALANCED, 9.0f / 16, 720, 1280));
  }

  @SmallTest
  public static void testLandscapeVideoInPortraitDisplay() {
    assertEquals(new Point(720, 405), getDisplaySize(SCALE_ASPECT_FIT, 16.0f / 9, 720, 1280));
    assertEquals(new Point(720, 1280), getDisplaySize(SCALE_ASPECT_FILL, 16.0f / 9, 720, 1280));
    assertEquals(new Point(720, 720), getDisplaySize(SCALE_ASPECT_BALANCED, 16.0f / 9, 720, 1280));
  }

  @SmallTest
  public static void testPortraitVideoInLandscapeDisplay() {
    assertEquals(new Point(405, 720), getDisplaySize(SCALE_ASPECT_FIT, 9.0f / 16, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FILL, 9.0f / 16, 1280, 720));
    assertEquals(new Point(720, 720), getDisplaySize(SCALE_ASPECT_BALANCED, 9.0f / 16, 1280, 720));
  }

  @SmallTest
  public static void testFourToThreeVideoInSixteenToNineDisplay() {
    assertEquals(new Point(960, 720), getDisplaySize(SCALE_ASPECT_FIT, 4.0f / 3, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_FILL, 4.0f / 3, 1280, 720));
    assertEquals(new Point(1280, 720), getDisplaySize(SCALE_ASPECT_BALANCED, 4.0f / 3, 1280, 720));
  }

  // Only keep 2 rounded decimals to make float comparison robust.
  private static double[] round(float[] array) {
    assertEquals(16, array.length);
    final double[] doubleArray = new double[16];
    for (int i = 0; i < 16; ++i) {
      doubleArray[i] = Math.round(100 * array[i]) / 100.0;
    }
    return doubleArray;
  }

  // Brief summary about matrix transformations:
  // A coordinate p = [u, v, 0, 1] is transformed by matrix m like this p' = [u', v', 0, 1] = m * p.
  // OpenGL uses column-major order, so:
  // u' = u * m[0] + v * m[4] + m[12].
  // v' = u * m[1] + v * m[5] + m[13].

  @SmallTest
  public static void testLayoutMatrixDefault() {
    final float layoutMatrix[] = getLayoutMatrix(false, 1.0f, 1.0f);
    // Assert:
    // u' = u.
    // v' = v.
    MoreAsserts.assertEquals(new double[] {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1}, round(layoutMatrix));
  }

  @SmallTest
  public static void testLayoutMatrixMirror() {
    final float layoutMatrix[] = getLayoutMatrix(true, 1.0f, 1.0f);
    // Assert:
    // u' = 1 - u.
    // v' = v.
    MoreAsserts.assertEquals(new double[] {
        -1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         1, 0, 0, 1}, round(layoutMatrix));
  }

  @SmallTest
  public static void testLayoutMatrixScale() {
    // Video has aspect ratio 2, but layout is square. This will cause only the center part of the
    // video to be visible, i.e. the u coordinate will go from 0.25 to 0.75 instead of from 0 to 1.
    final float layoutMatrix[] = getLayoutMatrix(false, 2.0f, 1.0f);
    // Assert:
    // u' = 0.25 + 0.5 u.
    // v' = v.
    MoreAsserts.assertEquals(new double[] {
         0.5, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 1, 0,
        0.25, 0, 0, 1}, round(layoutMatrix));
  }

  @SmallTest
  public static void testRotateTextureMatrixDefault() {
    // Test that rotation with 0 degrees returns an identical matrix.
    final float[] matrix = new float[] {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 0, 1, 2,
        3, 4, 5, 6
    };
    final float rotatedMatrix[] = rotateTextureMatrix(matrix, 0);
    MoreAsserts.assertEquals(round(matrix), round(rotatedMatrix));
  }

  @SmallTest
  public static void testRotateTextureMatrix90Deg() {
    final float samplingMatrix[] = rotateTextureMatrix(RendererCommon.identityMatrix(), 90);
    // Assert:
    // u' = 1 - v.
    // v' = u.
    MoreAsserts.assertEquals(new double[] {
         0, 1, 0, 0,
        -1, 0, 0, 0,
         0, 0, 1, 0,
         1, 0, 0, 1}, round(samplingMatrix));
  }

  @SmallTest
  public static void testRotateTextureMatrix180Deg() {
    final float samplingMatrix[] = rotateTextureMatrix(RendererCommon.identityMatrix(), 180);
    // Assert:
    // u' = 1 - u.
    // v' = 1 - v.
    MoreAsserts.assertEquals(new double[] {
        -1,  0, 0, 0,
         0, -1, 0, 0,
         0,  0, 1, 0,
         1,  1, 0, 1}, round(samplingMatrix));
  }
}
