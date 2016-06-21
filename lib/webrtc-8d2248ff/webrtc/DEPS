# Define rules for which include paths are allowed in our source.
include_rules = [
  # Base is only used to build Android APK tests and may not be referenced by
  # WebRTC production code.
  "-base",
  "-chromium",
  "+external/webrtc/webrtc",  # Android platform build.
  "+gflags",
  "+libyuv",
  "+testing",
  "-webrtc",  # Has to be disabled; otherwise all dirs below will be allowed.
  # Individual headers that will be moved out of here, see webrtc:
  "+webrtc/audio_receive_stream.h",
  "+webrtc/audio_send_stream.h",
  "+webrtc/audio_sink.h",
  "+webrtc/audio_state.h",
  "+webrtc/call.h",
  "+webrtc/common.h",
  "+webrtc/common_types.h",
  "+webrtc/config.h",
  "+webrtc/engine_configurations.h",
  "+webrtc/transport.h",
  "+webrtc/typedefs.h",
  "+webrtc/video_decoder.h",
  "+webrtc/video_encoder.h",
  "+webrtc/video_frame.h",
  "+webrtc/video_receive_stream.h",
  "+webrtc/video_renderer.h",
  "+webrtc/video_send_stream.h",

  "+WebRTC",
  "+webrtc/base",
  "+webrtc/modules/include",
  "+webrtc/test",
  "+webrtc/tools",
]

# The below rules will be removed when webrtc: is fixed.
specific_include_rules = {
  "audio_send_stream\.h": [
    "+webrtc/modules/audio_coding",
  ],
  "audio_receive_stream\.h": [
    "+webrtc/modules/audio_coding/codecs/audio_decoder_factory.h",
  ],
  "video_frame\.h": [
    "+webrtc/common_video",
  ],
  "video_receive_stream\.h": [
    "+webrtc/common_video/include",
    "+webrtc/media/base",
  ],
  "video_send_stream\.h": [
    "+webrtc/common_video/include",
    "+webrtc/media/base",
  ],
}
