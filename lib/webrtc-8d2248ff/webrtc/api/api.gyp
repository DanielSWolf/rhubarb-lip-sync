# Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [ '../build/common.gypi', ],
  'conditions': [
    ['os_posix == 1 and OS != "mac" and OS != "ios"', {
      'conditions': [
        ['sysroot!=""', {
          'variables': {
            'pkg-config': '../../../build/linux/pkg-config-wrapper "<(sysroot)" "<(target_arch)"',
          },
        }, {
          'variables': {
            'pkg-config': 'pkg-config'
          },
        }],
      ],
    }],
    # Excluded from the Chromium build since they cannot be built due to
    # incompability with Chromium's logging implementation.
    ['OS=="android" and build_with_chromium==0', {
      'targets': [
        {
          'target_name': 'libjingle_peerconnection_jni',
          'type': 'static_library',
          'dependencies': [
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:field_trial_default',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
            'libjingle_peerconnection',
          ],
          'sources': [
            'androidvideocapturer.cc',
            'androidvideocapturer.h',
            'java/jni/androidmediacodeccommon.h',
            'java/jni/androidmediadecoder_jni.cc',
            'java/jni/androidmediadecoder_jni.h',
            'java/jni/androidmediaencoder_jni.cc',
            'java/jni/androidmediaencoder_jni.h',
            'java/jni/androidmetrics_jni.cc',
            'java/jni/androidnetworkmonitor_jni.cc',
            'java/jni/androidnetworkmonitor_jni.h',
            'java/jni/androidvideocapturer_jni.cc',
            'java/jni/androidvideocapturer_jni.h',
            'java/jni/surfacetexturehelper_jni.cc',
            'java/jni/surfacetexturehelper_jni.h',
            'java/jni/classreferenceholder.cc',
            'java/jni/classreferenceholder.h',
            'java/jni/jni_helpers.cc',
            'java/jni/jni_helpers.h',
            'java/jni/native_handle_impl.cc',
            'java/jni/native_handle_impl.h',
            'java/jni/peerconnection_jni.cc',
          ],
          'include_dirs': [
            '<(libyuv_dir)/include',
          ],
          # TODO(kjellander): Make the code compile without disabling these flags.
          # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
          'cflags': [
            '-Wno-sign-compare',
            '-Wno-unused-variable',
          ],
          'cflags!': [
            '-Wextra',
          ],
          'msvs_disabled_warnings': [
            4245,  # conversion from 'int' to 'size_t', signed/unsigned mismatch.
            4267,  # conversion from 'size_t' to 'int', possible loss of data.
            4389,  # signed/unsigned mismatch.
          ],
        },
        {
          'target_name': 'libjingle_peerconnection_so',
          'type': 'shared_library',
          'dependencies': [
            'libjingle_peerconnection',
            'libjingle_peerconnection_jni',
          ],
          'sources': [
           'java/jni/jni_onload.cc',
          ],
          'variables': {
            # This library uses native JNI exports; tell GYP so that the
            # required symbols will be kept.
            'use_native_jni_exports': 1,
          },
        },
      ]
    }],
  ],  # conditions
  'targets': [
    {
      'target_name': 'libjingle_peerconnection',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/media/media.gyp:rtc_media',
        '<(webrtc_root)/pc/pc.gyp:rtc_pc',
      ],
      'sources': [
        'audiotrack.cc',
        'audiotrack.h',
        'datachannel.cc',
        'datachannel.h',
        'datachannelinterface.h',
        'dtlsidentitystore.h',
        'dtmfsender.cc',
        'dtmfsender.h',
        'dtmfsenderinterface.h',
        'jsep.h',
        'jsepicecandidate.cc',
        'jsepicecandidate.h',
        'jsepsessiondescription.cc',
        'jsepsessiondescription.h',
        'localaudiosource.cc',
        'localaudiosource.h',
        'mediaconstraintsinterface.cc',
        'mediaconstraintsinterface.h',
        'mediacontroller.cc',
        'mediacontroller.h',
        'mediastream.cc',
        'mediastream.h',
        'mediastreaminterface.h',
        'mediastreamobserver.cc',
        'mediastreamobserver.h',
        'mediastreamprovider.h',
        'mediastreamproxy.h',
        'mediastreamtrack.h',
        'mediastreamtrackproxy.h',
        'notifier.h',
        'peerconnection.cc',
        'peerconnection.h',
        'peerconnectionfactory.cc',
        'peerconnectionfactory.h',
        'peerconnectionfactoryproxy.h',
        'peerconnectioninterface.h',
        'peerconnectionproxy.h',
        'proxy.h',
        'remoteaudiosource.cc',
        'remoteaudiosource.h',
        'rtpparameters.h',
        'rtpreceiver.cc',
        'rtpreceiver.h',
        'rtpreceiverinterface.h',
        'rtpsender.cc',
        'rtpsender.h',
        'rtpsenderinterface.h',
        'sctputils.cc',
        'sctputils.h',
        'statscollector.cc',
        'statscollector.h',
        'statstypes.cc',
        'statstypes.h',
        'streamcollection.h',
        'videocapturertracksource.cc',
        'videocapturertracksource.h',
        'videosourceproxy.h',
        'videotrack.cc',
        'videotrack.h',
        'videotracksource.cc',
        'videotracksource.h',
        'webrtcsdp.cc',
        'webrtcsdp.h',
        'webrtcsession.cc',
        'webrtcsession.h',
        'webrtcsessiondescriptionfactory.cc',
        'webrtcsessiondescriptionfactory.h',
      ],
      'conditions': [
        ['clang==1', {
          'cflags!': [
            '-Wextra',
          ],
          'xcode_settings': {
            'WARNING_CFLAGS!': ['-Wextra'],
          },
        }, {
          'cflags': [
            '-Wno-maybe-uninitialized',  # Only exists for GCC.
          ],
        }],
        ['use_quic==1', {
          'dependencies': [
            '<(DEPTH)/third_party/libquic/libquic.gyp:libquic',
          ],
          'sources': [
            'quicdatachannel.cc',
            'quicdatachannel.h',
            'quicdatatransport.cc',
            'quicdatatransport.h',
          ],
          'export_dependent_settings': [
            '<(DEPTH)/third_party/libquic/libquic.gyp:libquic',
          ],
        }],
      ],
    },  # target libjingle_peerconnection
  ],  # targets
}
