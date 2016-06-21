# Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [ '../build/common.gypi', ],
  'targets': [
    {
      'target_name': 'peerconnection_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(webrtc_root)/api/api.gyp:libjingle_peerconnection',
        '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
        '<(webrtc_root)/common.gyp:webrtc_common',
        '<(webrtc_root)/media/media.gyp:rtc_unittest_main',
        '<(webrtc_root)/pc/pc.gyp:rtc_pc',
        '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
      ],
      'defines': [
        # Feature selection.
        'HAVE_SCTP',
      ],
      'sources': [
        'datachannel_unittest.cc',
        'dtmfsender_unittest.cc',
        'fakemetricsobserver.cc',
        'fakemetricsobserver.h',
        'jsepsessiondescription_unittest.cc',
        'localaudiosource_unittest.cc',
        'mediaconstraintsinterface_unittest.cc',
        'mediastream_unittest.cc',
        'peerconnection_unittest.cc',
        'peerconnectionendtoend_unittest.cc',
        'peerconnectionfactory_unittest.cc',
        'peerconnectioninterface_unittest.cc',
        'proxy_unittest.cc',
        'rtpsenderreceiver_unittest.cc',
        'statscollector_unittest.cc',
        'test/fakeaudiocapturemodule.cc',
        'test/fakeaudiocapturemodule.h',
        'test/fakeaudiocapturemodule_unittest.cc',
        'test/fakeconstraints.h',
        'test/fakedatachannelprovider.h',
        'test/fakeperiodicvideocapturer.h',
        'test/fakertccertificategenerator.h',
        'test/fakevideotrackrenderer.h',
        'test/mockpeerconnectionobservers.h',
        'test/peerconnectiontestwrapper.h',
        'test/peerconnectiontestwrapper.cc',
        'test/testsdpstrings.h',
        'videocapturertracksource_unittest.cc',
        'videotrack_unittest.cc',
        'webrtcsdp_unittest.cc',
        'webrtcsession_unittest.cc',
      ],
      # TODO(kjellander): Make the code compile without disabling these flags.
      # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
      'cflags': [
        '-Wno-sign-compare',
      ],
      'cflags!': [
        '-Wextra',
      ],
      'cflags_cc!': [
        '-Woverloaded-virtual',
      ],
      'msvs_disabled_warnings': [
        4245,  # conversion from 'int' to 'size_t', signed/unsigned mismatch.
        4267,  # conversion from 'size_t' to 'int', possible loss of data.
        4389,  # signed/unsigned mismatch.
      ],
      'conditions': [
        ['clang==1', {
          # TODO(kjellander): Make the code compile without disabling these flags.
          # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
          'cflags!': [
            '-Wextra',
          ],
          'xcode_settings': {
            'WARNING_CFLAGS!': ['-Wextra'],
          },
        }],
        ['OS=="android"', {
          'sources': [
            'test/androidtestinitializer.cc',
            'test/androidtestinitializer.h',
          ],
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
            '<(webrtc_root)/api/api.gyp:libjingle_peerconnection_jni',
          ],
        }],
        ['OS=="win" and clang==1', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                # Disable warnings failing when compiling with Clang on Windows.
                # https://bugs.chromium.org/p/webrtc/issues/detail?id=5366
                '-Wno-sign-compare',
                '-Wno-unused-function',
              ],
            },
          },
        }],
        ['use_quic==1', {
          'dependencies': [
            '<(DEPTH)/third_party/libquic/libquic.gyp:libquic',
          ],
          'sources': [
            'quicdatachannel_unittest.cc',
            'quicdatatransport_unittest.cc',
          ],
          'export_dependent_settings': [
            '<(DEPTH)/third_party/libquic/libquic.gyp:libquic',
          ],
        }],
      ],  # conditions
    },  # target peerconnection_unittests
  ],  # targets
  'conditions': [
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'libjingle_peerconnection_android_unittest',
          'type': 'none',
          'dependencies': [
            '<(webrtc_root)/api/api_java.gyp:libjingle_peerconnection_java',
          ],
          'variables': {
            'apk_name': 'libjingle_peerconnection_android_unittest',
            'java_in_dir': 'androidtests',
            'resource_dir': 'androidtests/res',
            'native_lib_target': 'libjingle_peerconnection_so',
            'is_test_apk': 1,
            'test_type': 'instrumentation',
            'tested_apk_path': '',
            'never_lint': 1,
          },
          'includes': [
            '../../build/java_apk.gypi',
            '../../build/android/test_runner.gypi',
          ],
        },
      ],  # targets
    }],  # OS=="android"
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'peerconnection_unittests_apk_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):peerconnection_unittests_apk',
          ],
        },
      ],
      'conditions': [
        ['test_isolation_mode != "noop"',
          {
            'targets': [
              {
                'target_name': 'peerconnection_unittests_apk_run',
                'type': 'none',
                'dependencies': [
                  '<(android_tests_path):peerconnection_unittests_apk',
                ],
                'includes': [
                  '../build/isolate.gypi',
                ],
                'sources': [
                  'peerconnection_unittests_apk.isolate',
                ],
              },
            ]
          }
        ],
      ],
    }],  # OS=="android"
    ['test_isolation_mode != "noop"', {
      'targets': [
        {
          'target_name': 'peerconnection_unittests_run',
          'type': 'none',
          'dependencies': [
            'peerconnection_unittests',
          ],
          'includes': [
            '../build/isolate.gypi',
          ],
          'sources': [
            'peerconnection_unittests.isolate',
          ],
        },
      ],  # targets
    }],  # test_isolation_mode != "noop"
  ],  # conditions
}
