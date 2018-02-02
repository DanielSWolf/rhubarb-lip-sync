/*
 *  Copyright 2014 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.appspot.apprtc.test;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import org.appspot.apprtc.AppRTCClient.SignalingParameters;
import org.appspot.apprtc.PeerConnectionClient;
import org.appspot.apprtc.PeerConnectionClient.PeerConnectionEvents;
import org.appspot.apprtc.PeerConnectionClient.PeerConnectionParameters;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.MediaCodecVideoEncoder;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.SessionDescription;
import org.webrtc.StatsReport;
import org.webrtc.VideoRenderer;

import android.os.Build;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

public class PeerConnectionClientTest extends InstrumentationTestCase
    implements PeerConnectionEvents {
  private static final String TAG = "RTCClientTest";
  private static final int ICE_CONNECTION_WAIT_TIMEOUT = 10000;
  private static final int WAIT_TIMEOUT = 7000;
  private static final int CAMERA_SWITCH_ATTEMPTS = 3;
  private static final int VIDEO_RESTART_ATTEMPTS = 3;
  private static final int CAPTURE_FORMAT_CHANGE_ATTEMPTS = 3;
  private static final int VIDEO_RESTART_TIMEOUT = 500;
  private static final int EXPECTED_VIDEO_FRAMES = 10;
  private static final String VIDEO_CODEC_VP8 = "VP8";
  private static final String VIDEO_CODEC_VP9 = "VP9";
  private static final String VIDEO_CODEC_H264 = "H264";
  private static final int AUDIO_RUN_TIMEOUT = 1000;
  private static final String LOCAL_RENDERER_NAME = "Local renderer";
  private static final String REMOTE_RENDERER_NAME = "Remote renderer";

  private static final int MAX_VIDEO_FPS = 30;
  private static final int WIDTH_VGA = 640;
  private static final int HEIGHT_VGA = 480;
  private static final int WIDTH_QVGA = 320;
  private static final int HEIGHT_QVGA = 240;

  // The peer connection client is assumed to be thread safe in itself; the
  // reference is written by the test thread and read by worker threads.
  private volatile PeerConnectionClient pcClient;
  private volatile boolean loopback;

  // EGL context that can be used by hardware video decoders to decode to a texture.
  private EglBase eglBase;

  // These are protected by their respective event objects.
  private ExecutorService signalingExecutor;
  private boolean isClosed;
  private boolean isIceConnected;
  private SessionDescription localSdp;
  private List<IceCandidate> iceCandidates = new LinkedList<IceCandidate>();
  private final Object localSdpEvent = new Object();
  private final Object iceCandidateEvent = new Object();
  private final Object iceConnectedEvent = new Object();
  private final Object closeEvent = new Object();

  // Mock renderer implementation.
  private static class MockRenderer implements VideoRenderer.Callbacks {
    // These are protected by 'this' since we gets called from worker threads.
    private String rendererName;
    private boolean renderFrameCalled = false;

    // Thread-safe in itself.
    private CountDownLatch doneRendering;

    public MockRenderer(int expectedFrames, String rendererName) {
      this.rendererName = rendererName;
      reset(expectedFrames);
    }

    // Resets render to wait for new amount of video frames.
    public synchronized void reset(int expectedFrames) {
      renderFrameCalled = false;
      doneRendering = new CountDownLatch(expectedFrames);
    }

    @Override
    public synchronized void renderFrame(VideoRenderer.I420Frame frame) {
      if (!renderFrameCalled) {
        if (rendererName != null) {
          Log.d(TAG, rendererName + " render frame: "
              + frame.rotatedWidth() + " x " + frame.rotatedHeight());
        } else {
          Log.d(TAG, "Render frame: " + frame.rotatedWidth() + " x " + frame.rotatedHeight());
        }
      }
      renderFrameCalled = true;
      VideoRenderer.renderFrameDone(frame);
      doneRendering.countDown();
    }


    // This method shouldn't hold any locks or touch member variables since it
    // blocks.
    public boolean waitForFramesRendered(int timeoutMs)
        throws InterruptedException {
      doneRendering.await(timeoutMs, TimeUnit.MILLISECONDS);
      return (doneRendering.getCount() <= 0);
    }
  }

  // Peer connection events implementation.
  @Override
  public void onLocalDescription(SessionDescription sdp) {
    Log.d(TAG, "LocalSDP type: " + sdp.type);
    synchronized (localSdpEvent) {
      localSdp = sdp;
      localSdpEvent.notifyAll();
    }
  }

  @Override
  public void onIceCandidate(final IceCandidate candidate) {
    synchronized(iceCandidateEvent) {
      Log.d(TAG, "IceCandidate #" + iceCandidates.size() + " : " + candidate.toString());
      if (loopback) {
        // Loopback local ICE candidate in a separate thread to avoid adding
        // remote ICE candidate in a local ICE candidate callback.
        signalingExecutor.execute(new Runnable() {
          @Override
          public void run() {
            pcClient.addRemoteIceCandidate(candidate);
          }
        });
      }
      iceCandidates.add(candidate);
      iceCandidateEvent.notifyAll();
    }
  }

  @Override
  public void onIceCandidatesRemoved(final IceCandidate[] candidates) {
    // TODO(honghaiz): Add this for tests.
  }

  @Override
  public void onIceConnected() {
    Log.d(TAG, "ICE Connected");
    synchronized(iceConnectedEvent) {
      isIceConnected = true;
      iceConnectedEvent.notifyAll();
    }
  }

  @Override
  public void onIceDisconnected() {
    Log.d(TAG, "ICE Disconnected");
    synchronized(iceConnectedEvent) {
      isIceConnected = false;
      iceConnectedEvent.notifyAll();
    }
  }

  @Override
  public void onPeerConnectionClosed() {
    Log.d(TAG, "PeerConnection closed");
    synchronized(closeEvent) {
      isClosed = true;
      closeEvent.notifyAll();
    }
  }

  @Override
  public void onPeerConnectionError(String description) {
    fail("PC Error: " + description);
  }

  @Override
  public void onPeerConnectionStatsReady(StatsReport[] reports) {
  }

  // Helper wait functions.
  private boolean waitForLocalSDP(int timeoutMs)
      throws InterruptedException {
    synchronized(localSdpEvent) {
      if (localSdp == null) {
        localSdpEvent.wait(timeoutMs);
      }
      return (localSdp != null);
    }
  }

  private boolean waitForIceCandidates(int timeoutMs)
      throws InterruptedException {
    synchronized(iceCandidateEvent) {
      if (iceCandidates.size() == 0) {
        iceCandidateEvent.wait(timeoutMs);
      }
      return (iceCandidates.size() > 0);
    }
  }

  private boolean waitForIceConnected(int timeoutMs)
      throws InterruptedException {
    synchronized(iceConnectedEvent) {
      if (!isIceConnected) {
        iceConnectedEvent.wait(timeoutMs);
      }
      if (!isIceConnected) {
        Log.e(TAG, "ICE connection failure");
      }

      return isIceConnected;
    }
  }

  private boolean waitForPeerConnectionClosed(int timeoutMs)
      throws InterruptedException {
    synchronized(closeEvent) {
      if (!isClosed) {
        closeEvent.wait(timeoutMs);
      }
      return isClosed;
    }
  }

  PeerConnectionClient createPeerConnectionClient(
      MockRenderer localRenderer, MockRenderer remoteRenderer,
      PeerConnectionParameters peerConnectionParameters, EglBase.Context eglContext) {
    List<PeerConnection.IceServer> iceServers =
        new LinkedList<PeerConnection.IceServer>();
    SignalingParameters signalingParameters = new SignalingParameters(
        iceServers, true, // iceServers, initiator.
        null, null, null, // clientId, wssUrl, wssPostUrl.
        null, null); // offerSdp, iceCandidates.

    PeerConnectionClient client = PeerConnectionClient.getInstance();
    PeerConnectionFactory.Options options = new PeerConnectionFactory.Options();
    options.networkIgnoreMask = 0;
    options.disableNetworkMonitor = true;
    client.setPeerConnectionFactoryOptions(options);
    client.createPeerConnectionFactory(
        getInstrumentation().getContext(), peerConnectionParameters, this);
    client.createPeerConnection(eglContext, localRenderer, remoteRenderer, signalingParameters);
    client.createOffer();
    return client;
  }

  private PeerConnectionParameters createParametersForAudioCall() {
    PeerConnectionParameters peerConnectionParameters =
        new PeerConnectionParameters(
            false, /* videoCallEnabled */
            true, /* loopback */
            false, /* tracing */
            // Video codec parameters.
            0, /* videoWidth */
            0, /* videoHeight */
            0, /* videoFps */
            0, /* videoStartBitrate */
            "", /* videoCodec */
            true, /* videoCodecHwAcceleration */
            false, /* captureToToTexture */
            // Audio codec parameters.
            0, /* audioStartBitrate */
            "OPUS", /* audioCodec */
            false, /* noAudioProcessing */
            false, /* aecDump */
            false /* useOpenSLES */,
            false /* disableBuiltInAEC */);
    return peerConnectionParameters;
  }

  private PeerConnectionParameters createParametersForVideoCall(
      String videoCodec, boolean captureToTexture) {
    PeerConnectionParameters peerConnectionParameters =
        new PeerConnectionParameters(
            true, /* videoCallEnabled */
            true, /* loopback */
            false, /* tracing */
            // Video codec parameters.
            0, /* videoWidth */
            0, /* videoHeight */
            0, /* videoFps */
            0, /* videoStartBitrate */
            videoCodec, /* videoCodec */
            true, /* videoCodecHwAcceleration */
            captureToTexture, /* captureToToTexture */
            // Audio codec parameters.
            0, /* audioStartBitrate */
            "OPUS", /* audioCodec */
            false, /* noAudioProcessing */
            false, /* aecDump */
            false /* useOpenSLES */,
            false /* disableBuiltInAEC */);
    return peerConnectionParameters;
  }

  @Override
  public void setUp() {
    signalingExecutor = Executors.newSingleThreadExecutor();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      eglBase = EglBase.create();
    }
  }

  @Override
  public void tearDown() {
    signalingExecutor.shutdown();
    if (eglBase != null) {
      eglBase.release();
    }
  }

  @SmallTest
  public void testSetLocalOfferMakesVideoFlowLocally()
      throws InterruptedException {
    Log.d(TAG, "testSetLocalOfferMakesVideoFlowLocally");
    MockRenderer localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
    pcClient = createPeerConnectionClient(
        localRenderer, new MockRenderer(0, null),
        createParametersForVideoCall(VIDEO_CODEC_VP8, false), null);

    // Wait for local SDP and ice candidates set events.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));
    assertTrue("ICE candidates were not generated.",
        waitForIceCandidates(WAIT_TIMEOUT));

    // Check that local video frames were rendered.
    assertTrue("Local video frames were not rendered.",
        localRenderer.waitForFramesRendered(WAIT_TIMEOUT));

    pcClient.close();
    assertTrue("PeerConnection close event was not received.",
        waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testSetLocalOfferMakesVideoFlowLocally Done.");
  }

  private void doLoopbackTest(PeerConnectionParameters parameters, boolean decodeToTexure)
      throws InterruptedException {
    loopback = true;
    MockRenderer localRenderer = null;
    MockRenderer remoteRenderer = null;
    if (parameters.videoCallEnabled) {
      Log.d(TAG, "testLoopback for video " + parameters.videoCodec);
      localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
      remoteRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, REMOTE_RENDERER_NAME);
    } else {
      Log.d(TAG, "testLoopback for audio.");
    }
    pcClient = createPeerConnectionClient(localRenderer, remoteRenderer, parameters,
        decodeToTexure ? eglBase.getEglBaseContext() : null);

    // Wait for local SDP, rename it to answer and set as remote SDP.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));
    SessionDescription remoteSdp = new SessionDescription(
        SessionDescription.Type.fromCanonicalForm("answer"),
        localSdp.description);
    pcClient.setRemoteDescription(remoteSdp);

    // Wait for ICE connection.
    assertTrue("ICE connection failure.", waitForIceConnected(ICE_CONNECTION_WAIT_TIMEOUT));

    if (parameters.videoCallEnabled) {
      // Check that local and remote video frames were rendered.
      assertTrue("Local video frames were not rendered.",
          localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
      assertTrue("Remote video frames were not rendered.",
          remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    } else {
      // For audio just sleep for 1 sec.
      // TODO(glaznev): check how we can detect that remote audio was rendered.
      Thread.sleep(AUDIO_RUN_TIMEOUT);
    }

    pcClient.close();
    assertTrue(waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testLoopback done.");
  }

  @SmallTest
  public void testLoopbackAudio() throws InterruptedException {
    doLoopbackTest(createParametersForAudioCall(), false);
  }

  @SmallTest
  public void testLoopbackVp8() throws InterruptedException {
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_VP8, false), false);
  }

  @SmallTest
  public void testLoopbackVp9() throws InterruptedException {
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_VP9, false), false);
  }

  @SmallTest
  public void testLoopbackH264() throws InterruptedException {
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_H264, false), false);
  }

  @SmallTest
  public void testLoopbackVp8DecodeToTexture() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Decode to textures is not supported, requires SDK version 19.");
      return;
    }
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_VP8, false), true);
  }

  @SmallTest
  public void testLoopbackVp9DecodeToTexture() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Decode to textures is not supported, requires SDK version 19.");
      return;
    }
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_VP9, false), true);
  }

  @SmallTest
  public void testLoopbackH264DecodeToTexture() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Decode to textures is not supported, requires SDK version 19.");
      return;
    }
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_H264, false), true);
  }

  @SmallTest
  public void testLoopbackVp8CaptureToTexture() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Encode to textures is not supported. Requires SDK version 19");
      return;
    }
    // TODO(perkj): If we can always capture to textures, there is no need to check if the
    // hardware encoder supports to encode from a texture.
    if (!MediaCodecVideoEncoder.isVp8HwSupportedUsingTextures()) {
      Log.i(TAG, "VP8 encode to textures is not supported.");
      return;
    }
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_VP8, true), true);
  }

  // Test that a call can be setup even if the EGL context used during initialization is
  // released before the Video codecs are created. The HW encoder and decoder is setup to use
  // textures.
  @SmallTest
  public void testLoopbackEglContextReleasedAfterCreatingPc() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Decode to textures is not supported. Requires SDK version 19");
      return;
    }

    loopback = true;
    PeerConnectionParameters parameters = createParametersForVideoCall(VIDEO_CODEC_VP8, true);
    MockRenderer localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
    MockRenderer remoteRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, REMOTE_RENDERER_NAME);
    pcClient = createPeerConnectionClient(
        localRenderer, remoteRenderer, parameters, eglBase.getEglBaseContext());

    // Wait for local SDP, rename it to answer and set as remote SDP.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));

    // Release the EGL context used for creating the PeerConnectionClient.
    // Since createPeerConnectionClient is asynchronous, we must wait for the local
    // SessionDescription.
    eglBase.release();
    eglBase = null;

    SessionDescription remoteSdp = new SessionDescription(
        SessionDescription.Type.fromCanonicalForm("answer"),
        localSdp.description);
    pcClient.setRemoteDescription(remoteSdp);

    // Wait for ICE connection.
    assertTrue("ICE connection failure.", waitForIceConnected(ICE_CONNECTION_WAIT_TIMEOUT));
    // Check that local and remote video frames were rendered.
    assertTrue("Local video frames were not rendered.",
        localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    assertTrue("Remote video frames were not rendered.",
        remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));

    pcClient.close();
    assertTrue(waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testLoopback done.");
  }

  @SmallTest
  public void testLoopbackH264CaptureToTexture() throws InterruptedException {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      Log.i(TAG, "Encode to textures is not supported. Requires KITKAT");
      return;
    }
    // TODO(perkj): If we can always capture to textures, there is no need to check if the
    // hardware encoder supports to encode from a texture.
    if (!MediaCodecVideoEncoder.isH264HwSupportedUsingTextures()) {
      Log.i(TAG, "H264 encode to textures is not supported.");
      return;
    }
    doLoopbackTest(createParametersForVideoCall(VIDEO_CODEC_H264, true), true);
  }


  // Checks if default front camera can be switched to back camera and then
  // again to front camera.
  @SmallTest
  public void testCameraSwitch() throws InterruptedException {
    Log.d(TAG, "testCameraSwitch");
    loopback = true;

    MockRenderer localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
    MockRenderer remoteRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, REMOTE_RENDERER_NAME);

    pcClient = createPeerConnectionClient(
        localRenderer, remoteRenderer, createParametersForVideoCall(VIDEO_CODEC_VP8, false), null);

    // Wait for local SDP, rename it to answer and set as remote SDP.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));
    SessionDescription remoteSdp = new SessionDescription(
        SessionDescription.Type.fromCanonicalForm("answer"),
        localSdp.description);
    pcClient.setRemoteDescription(remoteSdp);

    // Wait for ICE connection.
    assertTrue("ICE connection failure.", waitForIceConnected(ICE_CONNECTION_WAIT_TIMEOUT));

    // Check that local and remote video frames were rendered.
    assertTrue("Local video frames were not rendered before camera switch.",
        localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    assertTrue("Remote video frames were not rendered before camera switch.",
        remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));

    for (int i = 0; i < CAMERA_SWITCH_ATTEMPTS; i++) {
      // Try to switch camera
      pcClient.switchCamera();

      // Reset video renders and check that local and remote video frames
      // were rendered after camera switch.
      localRenderer.reset(EXPECTED_VIDEO_FRAMES);
      remoteRenderer.reset(EXPECTED_VIDEO_FRAMES);
      assertTrue("Local video frames were not rendered after camera switch.",
          localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
      assertTrue("Remote video frames were not rendered after camera switch.",
          remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    }
    pcClient.close();
    assertTrue(waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testCameraSwitch done.");
  }

  // Checks if video source can be restarted - simulate app goes to
  // background and back to foreground.
  @SmallTest
  public void testVideoSourceRestart() throws InterruptedException {
    Log.d(TAG, "testVideoSourceRestart");
    loopback = true;

    MockRenderer localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
    MockRenderer remoteRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, REMOTE_RENDERER_NAME);

    pcClient = createPeerConnectionClient(
        localRenderer, remoteRenderer, createParametersForVideoCall(VIDEO_CODEC_VP8, false), null);

    // Wait for local SDP, rename it to answer and set as remote SDP.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));
    SessionDescription remoteSdp = new SessionDescription(
        SessionDescription.Type.fromCanonicalForm("answer"),
        localSdp.description);
    pcClient.setRemoteDescription(remoteSdp);

    // Wait for ICE connection.
    assertTrue("ICE connection failure.", waitForIceConnected(ICE_CONNECTION_WAIT_TIMEOUT));

    // Check that local and remote video frames were rendered.
    assertTrue("Local video frames were not rendered before video restart.",
        localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    assertTrue("Remote video frames were not rendered before video restart.",
        remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));

    // Stop and then start video source a few times.
    for (int i = 0; i < VIDEO_RESTART_ATTEMPTS; i++) {
      pcClient.stopVideoSource();
      Thread.sleep(VIDEO_RESTART_TIMEOUT);
      pcClient.startVideoSource();

      // Reset video renders and check that local and remote video frames
      // were rendered after video restart.
      localRenderer.reset(EXPECTED_VIDEO_FRAMES);
      remoteRenderer.reset(EXPECTED_VIDEO_FRAMES);
      assertTrue("Local video frames were not rendered after video restart.",
          localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
      assertTrue("Remote video frames were not rendered after video restart.",
          remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    }
    pcClient.close();
    assertTrue(waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testVideoSourceRestart done.");
  }

  // Checks if capture format can be changed on fly and decoder can be reset properly.
  @SmallTest
  public void testCaptureFormatChange() throws InterruptedException {
    Log.d(TAG, "testCaptureFormatChange");
    loopback = true;

    MockRenderer localRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, LOCAL_RENDERER_NAME);
    MockRenderer remoteRenderer = new MockRenderer(EXPECTED_VIDEO_FRAMES, REMOTE_RENDERER_NAME);

    pcClient = createPeerConnectionClient(
        localRenderer, remoteRenderer, createParametersForVideoCall(VIDEO_CODEC_VP8, false), null);

    // Wait for local SDP, rename it to answer and set as remote SDP.
    assertTrue("Local SDP was not set.", waitForLocalSDP(WAIT_TIMEOUT));
    SessionDescription remoteSdp = new SessionDescription(
        SessionDescription.Type.fromCanonicalForm("answer"),
        localSdp.description);
    pcClient.setRemoteDescription(remoteSdp);

    // Wait for ICE connection.
    assertTrue("ICE connection failure.", waitForIceConnected(ICE_CONNECTION_WAIT_TIMEOUT));

    // Check that local and remote video frames were rendered.
    assertTrue("Local video frames were not rendered before camera resolution change.",
        localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    assertTrue("Remote video frames were not rendered before camera resolution change.",
        remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));

    // Change capture output format a few times.
    for (int i = 0; i < 2 * CAPTURE_FORMAT_CHANGE_ATTEMPTS; i++) {
      if (i % 2 == 0) {
        pcClient.changeCaptureFormat(WIDTH_VGA, HEIGHT_VGA, MAX_VIDEO_FPS);
      } else {
        pcClient.changeCaptureFormat(WIDTH_QVGA, HEIGHT_QVGA, MAX_VIDEO_FPS);
      }

      // Reset video renders and check that local and remote video frames
      // were rendered after capture format change.
      localRenderer.reset(EXPECTED_VIDEO_FRAMES);
      remoteRenderer.reset(EXPECTED_VIDEO_FRAMES);
      assertTrue("Local video frames were not rendered after capture format change.",
          localRenderer.waitForFramesRendered(WAIT_TIMEOUT));
      assertTrue("Remote video frames were not rendered after capture format change.",
          remoteRenderer.waitForFramesRendered(WAIT_TIMEOUT));
    }

    pcClient.close();
    assertTrue(waitForPeerConnectionClosed(WAIT_TIMEOUT));
    Log.d(TAG, "testCaptureFormatChange done.");
  }

}
