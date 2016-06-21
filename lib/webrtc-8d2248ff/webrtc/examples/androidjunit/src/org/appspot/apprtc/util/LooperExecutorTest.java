/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.appspot.apprtc.util;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.robolectric.Robolectric.shadowOf;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class LooperExecutorTest {
  private final static int RUN_TIMES = 10;

  @Mock private Runnable mockRunnable;
  private LooperExecutor executor;

  @Before
  public void setUp() {
    MockitoAnnotations.initMocks(this);
    executor = new LooperExecutor();
  }

  @After
  public void tearDown() {
    executor.requestStop();
    executePendingRunnables();
  }

  @Test
  public void testExecute() {
    executor.requestStart();

    for (int i = 0; i < RUN_TIMES; i++) {
      executor.execute(mockRunnable);
    }

    verifyNoMoreInteractions(mockRunnable);
    executePendingRunnables();
    verify(mockRunnable, times(RUN_TIMES)).run();
  }

  /**
   * Test that runnables executed before requestStart are ignored.
   */
  @Test
  public void testExecuteBeforeStart() {
    executor.execute(mockRunnable);

    executor.requestStart();
    executePendingRunnables();

    verifyNoMoreInteractions(mockRunnable);
  }

  /**
   * Test that runnables executed after requestStop are not executed.
   */
  @Test
  public void testExecuteAfterStop() {
    executor.requestStart();
    executor.requestStop();

    executor.execute(mockRunnable);
    executePendingRunnables();

    verifyNoMoreInteractions(mockRunnable);
  }

  /**
   * Test multiple requestStart calls are just ignored.
   */
  @Test
  public void testMultipleStarts() {
    executor.requestStart();
    testExecute();
  }

  /**
   * Test multiple requestStop calls are just ignored.
   */
  @Test
  public void testMultipleStops() {
    executor.requestStart();
    executor.requestStop();
    executor.requestStop();
    executePendingRunnables();
  }

  /**
   * Calls ShadowLooper's idle method in order to execute pending runnables.
   */
  private void executePendingRunnables() {
    ShadowLooper shadowLooper = getShadowLooper();
    shadowLooper.idle();
  }

  /**
   * Get ShadowLooper of the executor thread.
   */
  private ShadowLooper getShadowLooper() {
    return shadowOf(executor.getHandler().getLooper());
  }
}
