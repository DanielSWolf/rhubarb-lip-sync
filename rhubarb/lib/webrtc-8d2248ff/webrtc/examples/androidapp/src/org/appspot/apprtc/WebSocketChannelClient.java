/*
 *  Copyright 2014 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.appspot.apprtc;

import org.appspot.apprtc.util.AsyncHttpURLConnection;
import org.appspot.apprtc.util.AsyncHttpURLConnection.AsyncHttpEvents;
import org.appspot.apprtc.util.LooperExecutor;

import android.util.Log;

import de.tavendo.autobahn.WebSocket.WebSocketConnectionObserver;
import de.tavendo.autobahn.WebSocketConnection;
import de.tavendo.autobahn.WebSocketException;

import org.json.JSONException;
import org.json.JSONObject;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.LinkedList;

/**
 * WebSocket client implementation.
 *
 * <p>All public methods should be called from a looper executor thread
 * passed in a constructor, otherwise exception will be thrown.
 * All events are dispatched on the same thread.
 */

public class WebSocketChannelClient {
  private static final String TAG = "WSChannelRTCClient";
  private static final int CLOSE_TIMEOUT = 1000;
  private final WebSocketChannelEvents events;
  private final LooperExecutor executor;
  private WebSocketConnection ws;
  private WebSocketObserver wsObserver;
  private String wsServerUrl;
  private String postServerUrl;
  private String roomID;
  private String clientID;
  private WebSocketConnectionState state;
  private final Object closeEventLock = new Object();
  private boolean closeEvent;
  // WebSocket send queue. Messages are added to the queue when WebSocket
  // client is not registered and are consumed in register() call.
  private final LinkedList<String> wsSendQueue;

  /**
   * Possible WebSocket connection states.
   */
  public enum WebSocketConnectionState {
    NEW, CONNECTED, REGISTERED, CLOSED, ERROR
  };

  /**
   * Callback interface for messages delivered on WebSocket.
   * All events are dispatched from a looper executor thread.
   */
  public interface WebSocketChannelEvents {
    void onWebSocketMessage(final String message);
    void onWebSocketClose();
    void onWebSocketError(final String description);
  }

  public WebSocketChannelClient(LooperExecutor executor, WebSocketChannelEvents events) {
    this.executor = executor;
    this.events = events;
    roomID = null;
    clientID = null;
    wsSendQueue = new LinkedList<String>();
    state = WebSocketConnectionState.NEW;
  }

  public WebSocketConnectionState getState() {
    return state;
  }

  public void connect(final String wsUrl, final String postUrl) {
    checkIfCalledOnValidThread();
    if (state != WebSocketConnectionState.NEW) {
      Log.e(TAG, "WebSocket is already connected.");
      return;
    }
    wsServerUrl = wsUrl;
    postServerUrl = postUrl;
    closeEvent = false;

    Log.d(TAG, "Connecting WebSocket to: " + wsUrl + ". Post URL: " + postUrl);
    ws = new WebSocketConnection();
    wsObserver = new WebSocketObserver();
    try {
      ws.connect(new URI(wsServerUrl), wsObserver);
    } catch (URISyntaxException e) {
      reportError("URI error: " + e.getMessage());
    } catch (WebSocketException e) {
      reportError("WebSocket connection error: " + e.getMessage());
    }
  }

  public void register(final String roomID, final String clientID) {
    checkIfCalledOnValidThread();
    this.roomID = roomID;
    this.clientID = clientID;
    if (state != WebSocketConnectionState.CONNECTED) {
      Log.w(TAG, "WebSocket register() in state " + state);
      return;
    }
    Log.d(TAG, "Registering WebSocket for room " + roomID + ". ClientID: " + clientID);
    JSONObject json = new JSONObject();
    try {
      json.put("cmd", "register");
      json.put("roomid", roomID);
      json.put("clientid", clientID);
      Log.d(TAG, "C->WSS: " + json.toString());
      ws.sendTextMessage(json.toString());
      state = WebSocketConnectionState.REGISTERED;
      // Send any previously accumulated messages.
      for (String sendMessage : wsSendQueue) {
        send(sendMessage);
      }
      wsSendQueue.clear();
    } catch (JSONException e) {
      reportError("WebSocket register JSON error: " + e.getMessage());
    }
  }

  public void send(String message) {
    checkIfCalledOnValidThread();
    switch (state) {
      case NEW:
      case CONNECTED:
        // Store outgoing messages and send them after websocket client
        // is registered.
        Log.d(TAG, "WS ACC: " + message);
        wsSendQueue.add(message);
        return;
      case ERROR:
      case CLOSED:
        Log.e(TAG, "WebSocket send() in error or closed state : " + message);
        return;
      case REGISTERED:
        JSONObject json = new JSONObject();
        try {
          json.put("cmd", "send");
          json.put("msg", message);
          message = json.toString();
          Log.d(TAG, "C->WSS: " + message);
          ws.sendTextMessage(message);
        } catch (JSONException e) {
          reportError("WebSocket send JSON error: " + e.getMessage());
        }
        break;
    }
  }

  // This call can be used to send WebSocket messages before WebSocket
  // connection is opened.
  public void post(String message) {
    checkIfCalledOnValidThread();
    sendWSSMessage("POST", message);
  }

  public void disconnect(boolean waitForComplete) {
    checkIfCalledOnValidThread();
    Log.d(TAG, "Disconnect WebSocket. State: " + state);
    if (state == WebSocketConnectionState.REGISTERED) {
      // Send "bye" to WebSocket server.
      send("{\"type\": \"bye\"}");
      state = WebSocketConnectionState.CONNECTED;
      // Send http DELETE to http WebSocket server.
      sendWSSMessage("DELETE", "");
    }
    // Close WebSocket in CONNECTED or ERROR states only.
    if (state == WebSocketConnectionState.CONNECTED
        || state == WebSocketConnectionState.ERROR) {
      ws.disconnect();
      state = WebSocketConnectionState.CLOSED;

      // Wait for websocket close event to prevent websocket library from
      // sending any pending messages to deleted looper thread.
      if (waitForComplete) {
        synchronized (closeEventLock) {
          while (!closeEvent) {
            try {
              closeEventLock.wait(CLOSE_TIMEOUT);
              break;
            } catch (InterruptedException e) {
              Log.e(TAG, "Wait error: " + e.toString());
            }
          }
        }
      }
    }
    Log.d(TAG, "Disconnecting WebSocket done.");
  }

  private void reportError(final String errorMessage) {
    Log.e(TAG, errorMessage);
    executor.execute(new Runnable() {
      @Override
      public void run() {
        if (state != WebSocketConnectionState.ERROR) {
          state = WebSocketConnectionState.ERROR;
          events.onWebSocketError(errorMessage);
        }
      }
    });
  }

  // Asynchronously send POST/DELETE to WebSocket server.
  private void sendWSSMessage(final String method, final String message) {
    String postUrl = postServerUrl + "/" + roomID + "/" + clientID;
    Log.d(TAG, "WS " + method + " : " + postUrl + " : " + message);
    AsyncHttpURLConnection httpConnection = new AsyncHttpURLConnection(
        method, postUrl, message, new AsyncHttpEvents() {
          @Override
          public void onHttpError(String errorMessage) {
            reportError("WS " + method + " error: " + errorMessage);
          }

          @Override
          public void onHttpComplete(String response) {
          }
        });
    httpConnection.send();
  }

   // Helper method for debugging purposes. Ensures that WebSocket method is
   // called on a looper thread.
  private void checkIfCalledOnValidThread() {
    if (!executor.checkOnLooperThread()) {
      throw new IllegalStateException(
          "WebSocket method is not called on valid thread");
    }
  }

  private class WebSocketObserver implements WebSocketConnectionObserver {
    @Override
    public void onOpen() {
      Log.d(TAG, "WebSocket connection opened to: " + wsServerUrl);
      executor.execute(new Runnable() {
        @Override
        public void run() {
          state = WebSocketConnectionState.CONNECTED;
          // Check if we have pending register request.
          if (roomID != null && clientID != null) {
            register(roomID, clientID);
          }
        }
      });
    }

    @Override
    public void onClose(WebSocketCloseNotification code, String reason) {
      Log.d(TAG, "WebSocket connection closed. Code: " + code
          + ". Reason: " + reason + ". State: " + state);
      synchronized (closeEventLock) {
        closeEvent = true;
        closeEventLock.notify();
      }
      executor.execute(new Runnable() {
        @Override
        public void run() {
          if (state != WebSocketConnectionState.CLOSED) {
            state = WebSocketConnectionState.CLOSED;
            events.onWebSocketClose();
          }
        }
      });
    }

    @Override
    public void onTextMessage(String payload) {
      Log.d(TAG, "WSS->C: " + payload);
      final String message = payload;
      executor.execute(new Runnable() {
        @Override
        public void run() {
          if (state == WebSocketConnectionState.CONNECTED
              || state == WebSocketConnectionState.REGISTERED) {
            events.onWebSocketMessage(message);
          }
        }
      });
    }

    @Override
    public void onRawTextMessage(byte[] payload) {
    }

    @Override
    public void onBinaryMessage(byte[] payload) {
    }
  }

}
