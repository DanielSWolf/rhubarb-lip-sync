#
# libjingle
# Copyright 2012 Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'includes': ['../../build/common.gypi'],
  'conditions': [
    ['OS=="ios" or (OS=="mac" and target_arch!="ia32")', {
      # The >= 10.7 above is required for ARC.
      'targets': [
        {
          'target_name': 'libjingle_peerconnection_objc',
          'type': 'static_library',
          'dependencies': [
            '<(webrtc_root)/api/api.gyp:libjingle_peerconnection',
          ],
          'sources': [
            'objc/RTCAudioTrack+Internal.h',
            'objc/RTCAudioTrack.mm',
            'objc/RTCDataChannel+Internal.h',
            'objc/RTCDataChannel.mm',
            'objc/RTCEnumConverter.h',
            'objc/RTCEnumConverter.mm',
            'objc/RTCI420Frame+Internal.h',
            'objc/RTCI420Frame.mm',
            'objc/RTCICECandidate+Internal.h',
            'objc/RTCICECandidate.mm',
            'objc/RTCICEServer+Internal.h',
            'objc/RTCICEServer.mm',
            'objc/RTCLogging.mm',
            'objc/RTCMediaConstraints+Internal.h',
            'objc/RTCMediaConstraints.mm',
            'objc/RTCMediaConstraintsNative.cc',
            'objc/RTCMediaConstraintsNative.h',
            'objc/RTCMediaSource+Internal.h',
            'objc/RTCMediaSource.mm',
            'objc/RTCMediaStream+Internal.h',
            'objc/RTCMediaStream.mm',
            'objc/RTCMediaStreamTrack+Internal.h',
            'objc/RTCMediaStreamTrack.mm',
            'objc/RTCOpenGLVideoRenderer.mm',
            'objc/RTCPair.m',
            'objc/RTCPeerConnection+Internal.h',
            'objc/RTCPeerConnection.mm',
            'objc/RTCPeerConnectionFactory.mm',
            'objc/RTCPeerConnectionInterface+Internal.h',
            'objc/RTCPeerConnectionInterface.mm',
            'objc/RTCPeerConnectionObserver.h',
            'objc/RTCPeerConnectionObserver.mm',
            'objc/RTCSessionDescription+Internal.h',
            'objc/RTCSessionDescription.mm',
            'objc/RTCStatsReport+Internal.h',
            'objc/RTCStatsReport.mm',
            'objc/RTCVideoCapturer+Internal.h',
            'objc/RTCVideoCapturer.mm',
            'objc/RTCVideoRendererAdapter.h',
            'objc/RTCVideoRendererAdapter.mm',
            'objc/RTCVideoSource+Internal.h',
            'objc/RTCVideoSource.mm',
            'objc/RTCVideoTrack+Internal.h',
            'objc/RTCVideoTrack.mm',
            'objc/public/RTCAudioSource.h',
            'objc/public/RTCAudioTrack.h',
            'objc/public/RTCDataChannel.h',
            'objc/public/RTCFileLogger.h',
            'objc/public/RTCI420Frame.h',
            'objc/public/RTCICECandidate.h',
            'objc/public/RTCICEServer.h',
            'objc/public/RTCLogging.h',
            'objc/public/RTCMediaConstraints.h',
            'objc/public/RTCMediaSource.h',
            'objc/public/RTCMediaStream.h',
            'objc/public/RTCMediaStreamTrack.h',
            'objc/public/RTCOpenGLVideoRenderer.h',
            'objc/public/RTCPair.h',
            'objc/public/RTCPeerConnection.h',
            'objc/public/RTCPeerConnectionDelegate.h',
            'objc/public/RTCPeerConnectionFactory.h',
            'objc/public/RTCPeerConnectionInterface.h',
            'objc/public/RTCSessionDescription.h',
            'objc/public/RTCSessionDescriptionDelegate.h',
            'objc/public/RTCStatsDelegate.h',
            'objc/public/RTCStatsReport.h',
            'objc/public/RTCTypes.h',
            'objc/public/RTCVideoCapturer.h',
            'objc/public/RTCVideoRenderer.h',
            'objc/public/RTCVideoSource.h',
            'objc/public/RTCVideoTrack.h',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '<(DEPTH)/talk/app/webrtc/objc/public',
            ],
          },
          'include_dirs': [
            '<(webrtc_root)/webrtc/api',
            '<(DEPTH)/talk/app/webrtc/objc',
            '<(DEPTH)/talk/app/webrtc/objc/public',
          ],
          'link_settings': {
            'libraries': [
              '-lstdc++',
            ],
          },
          'all_dependent_settings': {
            'xcode_settings': {
              'CLANG_ENABLE_OBJC_ARC': 'YES',
            },
          },
          'xcode_settings': {
            'CLANG_ENABLE_OBJC_ARC': 'YES',
            # common.gypi enables this for mac but we want this to be disabled
            # like it is for ios.
            'CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS': 'NO',
            # Disabled due to failing when compiled with -Wall, see
            # https://bugs.chromium.org/p/webrtc/issues/detail?id=5397
            'WARNING_CFLAGS': ['-Wno-unused-property-ivar'],
          },
          'conditions': [
            ['OS=="ios"', {
              'sources': [
                'objc/avfoundationvideocapturer.h',
                'objc/avfoundationvideocapturer.mm',
                'objc/RTCAVFoundationVideoSource+Internal.h',
                'objc/RTCAVFoundationVideoSource.mm',
                'objc/RTCEAGLVideoView.m',
                'objc/public/RTCEAGLVideoView.h',
                'objc/public/RTCAVFoundationVideoSource.h',
              ],
              'dependencies': [
                '<(webrtc_root)/sdk/sdk.gyp:rtc_sdk_common_objc',
              ],
              'link_settings': {
                'xcode_settings': {
                  'OTHER_LDFLAGS': [
                    '-framework CoreGraphics',
                    '-framework GLKit',
                    '-framework OpenGLES',
                    '-framework QuartzCore',
                  ],
                },
              },
            }],
            ['OS=="mac"', {
              'sources': [
                'objc/RTCNSGLVideoView.m',
                'objc/public/RTCNSGLVideoView.h',
              ],
              'xcode_settings': {
                # Need to build against 10.7 framework for full ARC support
                # on OSX.
                'MACOSX_DEPLOYMENT_TARGET' : '10.7',
                # RTCVideoTrack.mm uses code with partial availability.
                # https://code.google.com/p/webrtc/issues/detail?id=4695
                'WARNING_CFLAGS!': ['-Wpartial-availability'],
              },
              'link_settings': {
                'xcode_settings': {
                  'OTHER_LDFLAGS': [
                    '-framework Cocoa',
                    '-framework OpenGL',
                  ],
                },
              },
            }],
          ],
        },  # target libjingle_peerconnection_objc
      ],
    }],
  ],
}
