/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.appspot.apprtc.util;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Looper based executor class. This is needed because WebSocketClient from autobanh requires the
 * thread to have a looper. The class is used in WebSocketRTCClient/WebSocketChannelClient.
 */
public class LooperExecutor extends Thread implements Executor {
  private static final String TAG = "LooperExecutor";
  // Object used to signal that looper thread has started and Handler instance
  // associated with looper thread has been allocated.
  private final Object looperStartedEvent = new Object();
  private final List<Runnable> scheduledPeriodicRunnables = new LinkedList<Runnable>();
  private Handler handler = null;
  private boolean running = false;
  private long threadId;

  @Override
  public void run() {
    Looper.prepare();
    synchronized (looperStartedEvent) {
      Log.d(TAG, "Looper thread started.");
      handler = new Handler();
      threadId = Thread.currentThread().getId();
      looperStartedEvent.notify();
    }
    Looper.loop();
  }

  public synchronized void requestStart() {
    if (running) {
      return;
    }
    running = true;
    handler = null;
    start();
    // Wait for Hander allocation.
    synchronized (looperStartedEvent) {
      while (handler == null) {
        try {
          looperStartedEvent.wait();
        } catch (InterruptedException e) {
          Log.e(TAG, "Can not start looper thread");
          running = false;
        }
      }
    }
  }

  public synchronized void requestStop() {
    if (!running) {
      return;
    }
    running = false;
    handler.post(new Runnable() {
      @Override
      public void run() {
        handler.getLooper().quit();
        Log.d(TAG, "Looper thread finished.");
      }
    });
  }

  // Checks if current thread is a looper thread.
  public boolean checkOnLooperThread() {
    return (Thread.currentThread().getId() == threadId);
  }

  public synchronized void scheduleAtFixedRate(final Runnable command, final long periodMillis) {
    if (!running) {
      Log.w(TAG, "Trying to schedule task for non running executor");
      return;
    }
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (running) {
          command.run();
          if (!handler.postDelayed(this, periodMillis)) {
            Log.e(TAG, "Failed to post a delayed runnable in the chain.");
          }
        }
      }
    };
    scheduledPeriodicRunnables.add(runnable);
    if (!handler.postDelayed(runnable, periodMillis)) {
      Log.e(TAG, "Failed to post a delayed runnable.");
    }
  }

  public synchronized void cancelScheduledTasks() {
    if (!running) {
      Log.w(TAG, "Trying to cancel schedule tasks for non running executor");
      return;
    }

    // Stop scheduled periodic tasks.
    for (Runnable r : scheduledPeriodicRunnables) {
      handler.removeCallbacks(r);
    }
    scheduledPeriodicRunnables.clear();
  }

  @Override
  public synchronized void execute(final Runnable runnable) {
    if (!running) {
      Log.w(TAG, "Running looper executor without calling requestStart()");
      return;
    }
    if (Thread.currentThread().getId() == threadId) {
      runnable.run();
    } else {
      handler.post(runnable);
    }
  }

  /**
   * Access to the handler for testing purposes.
   */
  Handler getHandler() {
    return handler;
  }
}
