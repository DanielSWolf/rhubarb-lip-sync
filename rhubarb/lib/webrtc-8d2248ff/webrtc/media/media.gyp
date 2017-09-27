# Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
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
      'target_name': 'rtc_media',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/base/base.gyp:rtc_base_approved',
        '<(webrtc_root)/common.gyp:webrtc_common',
        '<(webrtc_root)/webrtc.gyp:webrtc',
        '<(webrtc_root)/voice_engine/voice_engine.gyp:voice_engine',
        '<(webrtc_root)/system_wrappers/system_wrappers.gyp:system_wrappers',
        '<(webrtc_root)/p2p/p2p.gyp:rtc_p2p',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(libyuv_dir)/include',
        ],
      },
      'sources': [
        'base/audiosource.h',
        'base/codec.cc',
        'base/codec.h',
        'base/cpuid.cc',
        'base/cpuid.h',
        'base/cryptoparams.h',
        'base/device.h',
        'base/fakescreencapturerfactory.h',
        'base/hybriddataengine.h',
        'base/mediachannel.h',
        'base/mediacommon.h',
        'base/mediaconstants.cc',
        'base/mediaconstants.h',
        'base/mediaengine.cc',
        'base/mediaengine.h',
        'base/rtpdataengine.cc',
        'base/rtpdataengine.h',
        'base/rtpdump.cc',
        'base/rtpdump.h',
        'base/rtputils.cc',
        'base/rtputils.h',
        'base/screencastid.h',
        'base/streamparams.cc',
        'base/streamparams.h',
        'base/turnutils.cc',
        'base/turnutils.h',
        'base/videoadapter.cc',
        'base/videoadapter.h',
        'base/videobroadcaster.cc',
        'base/videobroadcaster.h',
        'base/videocapturer.cc',
        'base/videocapturer.h',
        'base/videocapturerfactory.h',
        'base/videocommon.cc',
        'base/videocommon.h',
        'base/videoframe.cc',
        'base/videoframe.h',
        'base/videoframefactory.cc',
        'base/videoframefactory.h',
        'base/videosourcebase.cc',
        'base/videosourcebase.h',
        'devices/videorendererfactory.h',
        'engine/nullwebrtcvideoengine.h',
        'engine/simulcast.cc',
        'engine/simulcast.h',
        'engine/webrtccommon.h',
        'engine/webrtcmediaengine.cc',
        'engine/webrtcmediaengine.h',
        'engine/webrtcmediaengine.cc',
        'engine/webrtcvideocapturer.cc',
        'engine/webrtcvideocapturer.h',
        'engine/webrtcvideocapturerfactory.h',
        'engine/webrtcvideocapturerfactory.cc',
        'engine/webrtcvideodecoderfactory.h',
        'engine/webrtcvideoencoderfactory.h',
        'engine/webrtcvideoengine2.cc',
        'engine/webrtcvideoengine2.h',
        'engine/webrtcvideoframe.cc',
        'engine/webrtcvideoframe.h',
        'engine/webrtcvideoframefactory.cc',
        'engine/webrtcvideoframefactory.h',
        'engine/webrtcvoe.h',
        'engine/webrtcvoiceengine.cc',
        'engine/webrtcvoiceengine.h',
        'sctp/sctpdataengine.cc',
        'sctp/sctpdataengine.h',
      ],
      # TODO(kjellander): Make the code compile without disabling these flags.
      # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
      'cflags': [
        '-Wno-deprecated-declarations',
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
        ['build_libyuv==1', {
          'dependencies': ['<(DEPTH)/third_party/libyuv/libyuv.gyp:libyuv',],
        }],
        ['build_usrsctp==1', {
          'include_dirs': [
            # TODO(jiayl): move this into the direct_dependent_settings of
            # usrsctp.gyp.
            '<(DEPTH)/third_party/usrsctp/usrsctplib',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/usrsctp/usrsctp.gyp:usrsctplib',
          ],
        }],
        ['build_with_chromium==1', {
          'dependencies': [
            '<(webrtc_root)/modules/modules.gyp:video_capture',
          ],
        }, {
          'defines': [
            'HAVE_WEBRTC_VIDEO',
            'HAVE_WEBRTC_VOICE',
          ],
          'direct_dependent_settings': {
            'defines': [
              'HAVE_WEBRTC_VIDEO',
              'HAVE_WEBRTC_VOICE',
            ],
          },
          'dependencies': [
            '<(webrtc_root)/modules/modules.gyp:video_capture_module_internal_impl',
          ],
        }],
        ['OS=="linux" and use_gtk==1', {
          'sources': [
            'devices/gtkvideorenderer.cc',
            'devices/gtkvideorenderer.h',
          ],
          'cflags': [
            '<!@(pkg-config --cflags gobject-2.0 gthread-2.0 gtk+-2.0)',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'devices/gdivideorenderer.cc',
            'devices/gdivideorenderer.h',
          ],
          'msvs_settings': {
            'VCLibrarianTool': {
              'AdditionalDependencies': [
                'd3d9.lib',
                'gdi32.lib',
                'strmiids.lib',
              ],
            },
          },
        }],
        ['OS=="mac" and target_arch=="ia32"', {
          'sources': [
            'devices/carbonvideorenderer.cc',
            'devices/carbonvideorenderer.h',
          ],
          'link_settings': {
            'xcode_settings': {
              'OTHER_LDFLAGS': [
                '-framework Carbon',
              ],
            },
          },
        }],
        ['OS=="ios" or (OS=="mac" and target_arch!="ia32")', {
          'defines': [
            'CARBON_DEPRECATED=YES',
          ],
        }],
      ],
    },  # target rtc_media
  ],  # targets.
  'conditions': [
    ['include_tests==1', {
      'targets' : [
        {
          'target_name': 'rtc_unittest_main',
          'type': 'static_library',
          'dependencies': [
            '<(DEPTH)/testing/gmock.gyp:gmock',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '<(libyuv_dir)/include',
              '<(DEPTH)/testing/gmock/include',
            ],
          },
          'conditions': [
            ['build_libyuv==1', {
              'dependencies': ['<(DEPTH)/third_party/libyuv/libyuv.gyp:libyuv',],
            }],
            ['OS=="ios"', {
              # TODO(kjellander): Make the code compile without disabling these.
              # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
              'cflags': [
                '-Wno-unused-variable',
              ],
              'xcode_settings': {
                'WARNING_CFLAGS': [
                  '-Wno-unused-variable',
                ],
              },
            }],
          ],
          'include_dirs': [
             '<(DEPTH)/testing/gtest/include',
             '<(DEPTH)/testing/gtest',
           ],
          'sources': [
            'base/fakemediaengine.h',
            'base/fakenetworkinterface.h',
            'base/fakertp.h',
            'base/fakevideocapturer.h',
            'base/fakevideorenderer.h',
            'base/testutils.cc',
            'base/testutils.h',
            'engine/fakewebrtccall.cc',
            'engine/fakewebrtccall.h',
            'engine/fakewebrtccommon.h',
            'engine/fakewebrtcdeviceinfo.h',
            'engine/fakewebrtcvcmfactory.h',
            'engine/fakewebrtcvideocapturemodule.h',
            'engine/fakewebrtcvideoengine.h',
            'engine/fakewebrtcvoiceengine.h',
          ],
        },  # target rtc_unittest_main
        {
          'target_name': 'rtc_media_unittests',
          'type': 'executable',
          'dependencies': [
            '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
            '<(webrtc_root)/media/media.gyp:rtc_media',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
            '<(webrtc_root)/test/test.gyp:test_support',
            'rtc_unittest_main',
          ],
          'sources': [
            'base/codec_unittest.cc',
            'base/rtpdataengine_unittest.cc',
            'base/rtpdump_unittest.cc',
            'base/rtputils_unittest.cc',
            'base/streamparams_unittest.cc',
            'base/turnutils_unittest.cc',
            'base/videoadapter_unittest.cc',
            'base/videobroadcaster_unittest.cc',
            'base/videocapturer_unittest.cc',
            'base/videocommon_unittest.cc',
            'base/videoengine_unittest.h',
            'base/videoframe_unittest.h',
            'engine/nullwebrtcvideoengine_unittest.cc',
            'engine/simulcast_unittest.cc',
            'engine/webrtcmediaengine_unittest.cc',
            'engine/webrtcvideocapturer_unittest.cc',
            'engine/webrtcvideoframe_unittest.cc',
            'engine/webrtcvideoframefactory_unittest.cc',
            'engine/webrtcvideoengine2_unittest.cc',
            'engine/webrtcvoiceengine_unittest.cc',
            'sctp/sctpdataengine_unittest.cc',
          ],
          # TODO(kjellander): Make the code compile without disabling these flags.
          # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
          'cflags': [
            '-Wno-sign-compare',
          ],
          'cflags_cc!': [
            '-Woverloaded-virtual',
          ],
          'msvs_disabled_warnings': [
            4245,  # conversion from 'int' to 'uint32_t', signed/unsigned mismatch.
            4389,  # signed/unsigned mismatch.
          ],
          'conditions': [
            ['OS=="win"', {
              'msvs_settings': {
                'VCLinkerTool': {
                  'AdditionalDependencies': [
                    # TODO(ronghuawu): Since we've included strmiids in
                    # libjingle_media target, we shouldn't need this here.
                    # Find out why it doesn't work without this.
                    'strmiids.lib',
                  ],
                },
              },
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
            },],
            ['clang==1', {
              # TODO(kjellander): Make the code compile without disabling these.
              # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
              'cflags!': [
                '-Wextra',
              ],
              'xcode_settings': {
                'WARNING_CFLAGS!': ['-Wextra'],
              },
            }],
            ['OS=="ios"', {
              'mac_bundle_resources': [
                '<(DEPTH)/resources/media/captured-320x240-2s-48.frames',
                '<(DEPTH)/resources/media/faces.1280x720_P420.yuv',
                '<(DEPTH)/resources/media/faces_I420.jpg',
                '<(DEPTH)/resources/media/faces_I422.jpg',
                '<(DEPTH)/resources/media/faces_I444.jpg',
                '<(DEPTH)/resources/media/faces_I411.jpg',
                '<(DEPTH)/resources/media/faces_I400.jpg',
              ],
            }],
          ],
        },  # target rtc_media_unittests
      ],  # targets
      'conditions': [
        ['test_isolation_mode != "noop"', {
          'targets': [
            {
              'target_name': 'rtc_media_unittests_run',
              'type': 'none',
              'dependencies': [
                'rtc_media_unittests',
              ],
              'includes': [
                '../build/isolate.gypi',
              ],
              'sources': [
                'rtc_media_unittests.isolate',
              ],
            },
          ],
        }],
      ],  # conditions
    }],  # include_tests==1
  ],  # conditions
}
