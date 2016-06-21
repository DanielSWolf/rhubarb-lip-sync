# Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': ['../build/common.gypi'],
  'variables': {
    'rtc_pc_defines': [
      'SRTP_RELATIVE_PATH',
      'HAVE_SCTP',
      'HAVE_SRTP',
    ],
  },
  'targets': [
    {
      'target_name': 'rtc_pc',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/base/base.gyp:rtc_base',
        '<(webrtc_root)/media/media.gyp:rtc_media',
      ],
      'conditions': [
        ['build_with_chromium==1', {
          'sources': [
            'externalhmac.h',
            'externalhmac.cc',
          ],
        }],
        ['build_libsrtp==1', {
          'dependencies': [
            '<(DEPTH)/third_party/libsrtp/libsrtp.gyp:libsrtp',
          ],
        }],
      ],
      'defines': [
        '<@(rtc_pc_defines)',
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
      ],
      'direct_dependent_settings': {
        'defines': [
          '<@(rtc_pc_defines)'
        ],
        'include_dirs': [
          '<(DEPTH)/testing/gtest/include',
        ],
      },
      'sources': [
        'audiomonitor.cc',
        'audiomonitor.h',
        'bundlefilter.cc',
        'bundlefilter.h',
        'channel.cc',
        'channel.h',
        'channelmanager.cc',
        'channelmanager.h',
        'currentspeakermonitor.cc',
        'currentspeakermonitor.h',
        'mediamonitor.cc',
        'mediamonitor.h',
        'mediasession.cc',
        'mediasession.h',
        'mediasink.h',
        'rtcpmuxfilter.cc',
        'rtcpmuxfilter.h',
        'srtpfilter.cc',
        'srtpfilter.h',
        'voicechannel.h',
      ],
    },  # target rtc_pc
  ],  # targets
  'conditions': [
    ['include_tests==1', {
      'targets' : [
        {
          'target_name': 'rtc_pc_unittests',
          'type': 'executable',
          'dependencies': [
            '<(webrtc_root)/api/api.gyp:libjingle_peerconnection',
            '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
            '<(webrtc_root)/media/media.gyp:rtc_unittest_main',
            '<(webrtc_root)/pc/pc.gyp:rtc_pc',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
          ],
          'include_dirs': [
            '<(DEPTH)/third_party/libsrtp/srtp',
          ],
          'sources': [
            'bundlefilter_unittest.cc',
            'channel_unittest.cc',
            'channelmanager_unittest.cc',
            'currentspeakermonitor_unittest.cc',
            'mediasession_unittest.cc',
            'rtcpmuxfilter_unittest.cc',
            'srtpfilter_unittest.cc',
          ],
          'conditions': [
            ['clang==0', {
              'cflags': [
                '-Wno-maybe-uninitialized',  # Only exists for GCC.
              ],
            }],
            ['build_libsrtp==1', {
              'dependencies': [
                '<(DEPTH)/third_party/libsrtp/libsrtp.gyp:libsrtp',
              ],
            }],
            ['OS=="win"', {
              'msvs_settings': {
                'VCLinkerTool': {
                  'AdditionalDependencies': [
                    'strmiids.lib',
                  ],
                },
              },
            }],
          ],
        },  # target rtc_pc_unittests
      ],  # targets
      'conditions': [
        ['test_isolation_mode != "noop"', {
          'targets': [
            {
              'target_name': 'rtc_pc_unittests_run',
              'type': 'none',
              'dependencies': [
                'rtc_pc_unittests',
              ],
              'includes': [
                '../build/isolate.gypi',
              ],
              'sources': [
                'rtc_pc_unittests.isolate',
              ],
            },
          ],
        }],
      ],  # conditions
    }],  # include_tests==1
  ],  # conditions
}
