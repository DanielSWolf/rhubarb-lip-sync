# Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.
{
  'targets': [
    {
      'target_name': 'rtc_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        'base/base.gyp:rtc_base',
        'base/base.gyp:rtc_task_queue',
        'base/base_tests.gyp:rtc_base_tests_utils',
        'p2p/p2p.gyp:rtc_p2p',
        'p2p/p2p.gyp:libstunprober',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/testing/gmock.gyp:gmock',
      ],
      'sources': [
        'base/array_view_unittest.cc',
        'base/atomicops_unittest.cc',
        'base/autodetectproxy_unittest.cc',
        'base/bandwidthsmoother_unittest.cc',
        'base/base64_unittest.cc',
        'base/basictypes_unittest.cc',
        'base/bind_unittest.cc',
        'base/bitbuffer_unittest.cc',
        'base/buffer_unittest.cc',
        'base/bufferqueue_unittest.cc',
        'base/bytebuffer_unittest.cc',
        'base/byteorder_unittest.cc',
        'base/callback_unittest.cc',
        'base/copyonwritebuffer_unittest.cc',
        'base/crc32_unittest.cc',
        'base/criticalsection_unittest.cc',
        'base/event_tracer_unittest.cc',
        'base/event_unittest.cc',
        'base/exp_filter_unittest.cc',
        'base/filerotatingstream_unittest.cc',
        'base/fileutils_unittest.cc',
        'base/helpers_unittest.cc',
        'base/httpbase_unittest.cc',
        'base/httpcommon_unittest.cc',
        'base/httpserver_unittest.cc',
        'base/ipaddress_unittest.cc',
        'base/logging_unittest.cc',
        'base/md5digest_unittest.cc',
        'base/messagedigest_unittest.cc',
        'base/messagequeue_unittest.cc',
        'base/mod_ops_unittest.cc',
        'base/multipart_unittest.cc',
        'base/nat_unittest.cc',
        'base/network_unittest.cc',
        'base/onetimeevent_unittest.cc',
        'base/optional_unittest.cc',
        'base/optionsfile_unittest.cc',
        'base/pathutils_unittest.cc',
        'base/platform_thread_unittest.cc',
        'base/profiler_unittest.cc',
        'base/proxy_unittest.cc',
        'base/proxydetect_unittest.cc',
        'base/random_unittest.cc',
        'base/rate_statistics_unittest.cc',
        'base/ratelimiter_unittest.cc',
        'base/ratetracker_unittest.cc',
        'base/referencecountedsingletonfactory_unittest.cc',
        'base/rollingaccumulator_unittest.cc',
        'base/rtccertificate_unittest.cc',
        'base/rtccertificategenerator_unittest.cc',
        'base/scopedptrcollection_unittest.cc',
        'base/sha1digest_unittest.cc',
        'base/sharedexclusivelock_unittest.cc',
        'base/signalthread_unittest.cc',
        'base/sigslot_unittest.cc',
        'base/sigslottester.h',
        'base/sigslottester.h.pump',
        'base/stream_unittest.cc',
        'base/stringencode_unittest.cc',
        'base/stringutils_unittest.cc',
        'base/swap_queue_unittest.cc',
        # TODO(ronghuawu): Reenable this test.
        # 'systeminfo_unittest.cc',
        'base/task_queue_unittest.cc',
        'base/task_unittest.cc',
        'base/testclient_unittest.cc',
        'base/thread_checker_unittest.cc',
        'base/thread_unittest.cc',
        'base/timeutils_unittest.cc',
        'base/urlencode_unittest.cc',
        'base/versionparsing_unittest.cc',
        # TODO(ronghuawu): Reenable this test.
        # 'windowpicker_unittest.cc',
        'p2p/base/dtlstransportchannel_unittest.cc',
        'p2p/base/fakeportallocator.h',
        'p2p/base/faketransportcontroller.h',
        'p2p/base/p2ptransportchannel_unittest.cc',
        'p2p/base/port_unittest.cc',
        'p2p/base/portallocator_unittest.cc',
        'p2p/base/pseudotcp_unittest.cc',
        'p2p/base/relayport_unittest.cc',
        'p2p/base/relayserver_unittest.cc',
        'p2p/base/stun_unittest.cc',
        'p2p/base/stunport_unittest.cc',
        'p2p/base/stunrequest_unittest.cc',
        'p2p/base/stunserver_unittest.cc',
        'p2p/base/testrelayserver.h',
        'p2p/base/teststunserver.h',
        'p2p/base/testturnserver.h',
        'p2p/base/transport_unittest.cc',
        'p2p/base/transportcontroller_unittest.cc',
        'p2p/base/transportdescriptionfactory_unittest.cc',
        'p2p/base/tcpport_unittest.cc',
        'p2p/base/turnport_unittest.cc',
        'p2p/client/basicportallocator_unittest.cc',
        'p2p/stunprober/stunprober_unittest.cc',
      ],
      'conditions': [
       ['OS=="linux"', {
          'sources': [
            'base/latebindingsymboltable_unittest.cc',
            # TODO(ronghuawu): Reenable this test.
            # 'linux_unittest.cc',
            'base/linuxfdwalk_unittest.cc',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'base/win32_unittest.cc',
            'base/win32regkey_unittest.cc',
            'base/win32window_unittest.cc',
            'base/win32windowpicker_unittest.cc',
            'base/winfirewall_unittest.cc',
          ],
        }],
        ['OS=="win" and clang==1', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                # Disable warnings failing when compiling with Clang on Windows.
                # https://bugs.chromium.org/p/webrtc/issues/detail?id=5366
                '-Wno-missing-braces',
                '-Wno-sign-compare',
                '-Wno-unused-const-variable',
              ],
            },
          },
        }],
        ['OS=="mac"', {
          'sources': [
            'base/macutils_unittest.cc',
          ],
        }],
        ['os_posix==1', {
          'sources': [
            'base/ssladapter_unittest.cc',
            'base/sslidentity_unittest.cc',
            'base/sslstreamadapter_unittest.cc',
          ],
        }],
        ['OS=="ios" or (OS=="mac" and target_arch!="ia32")', {
          'defines': [
            'CARBON_DEPRECATED=YES',
          ],
        }],
        ['use_quic==1', {
          'sources': [
            'p2p/quic/quicconnectionhelper_unittest.cc',
            'p2p/quic/quicsession_unittest.cc',
            'p2p/quic/quictransport_unittest.cc',
            'p2p/quic/quictransportchannel_unittest.cc',
            'p2p/quic/reliablequicstream_unittest.cc',
          ],
        }],
        ['OS=="android"', {
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['OS=="ios" or (OS=="mac" and mac_deployment_target=="10.7")', {
          'includes': [
            'build/objc_common.gypi',
          ],
          'dependencies': [
            'sdk/sdk.gyp:rtc_sdk_peerconnection_objc',
            'system_wrappers/system_wrappers.gyp:metrics_default',
          ],
          'sources': [
            'sdk/objc/Framework/UnitTests/RTCConfigurationTest.mm',
            'sdk/objc/Framework/UnitTests/RTCDataChannelConfigurationTest.mm',
            'sdk/objc/Framework/UnitTests/RTCIceCandidateTest.mm',
            'sdk/objc/Framework/UnitTests/RTCIceServerTest.mm',
            'sdk/objc/Framework/UnitTests/RTCMediaConstraintsTest.mm',
            'sdk/objc/Framework/UnitTests/RTCSessionDescriptionTest.mm',
          ],
          'xcode_settings': {
            # |-ObjC| flag needed to make sure category method implementations
            # are included:
            # https://developer.apple.com/library/mac/qa/qa1490/_index.html
            'OTHER_LDFLAGS': ['-ObjC'],
          },
        }],
      ],
    },
    {
      'target_name': 'xmllite_xmpp_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        'base/base_tests.gyp:rtc_base_tests_utils',  # needed for main()
        'libjingle/xmllite/xmllite.gyp:rtc_xmllite',
        'libjingle/xmpp/xmpp.gyp:rtc_xmpp',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
      'sources': [
        'libjingle/xmllite/qname_unittest.cc',
        'libjingle/xmllite/xmlbuilder_unittest.cc',
        'libjingle/xmllite/xmlelement_unittest.cc',
        'libjingle/xmllite/xmlnsstack_unittest.cc',
        'libjingle/xmllite/xmlparser_unittest.cc',
        'libjingle/xmllite/xmlprinter_unittest.cc',
        'libjingle/xmpp/fakexmppclient.h',
        'libjingle/xmpp/hangoutpubsubclient_unittest.cc',
        'libjingle/xmpp/jid_unittest.cc',
        'libjingle/xmpp/mucroomconfigtask_unittest.cc',
        'libjingle/xmpp/mucroomdiscoverytask_unittest.cc',
        'libjingle/xmpp/mucroomlookuptask_unittest.cc',
        'libjingle/xmpp/mucroomuniquehangoutidtask_unittest.cc',
        'libjingle/xmpp/pingtask_unittest.cc',
        'libjingle/xmpp/pubsubclient_unittest.cc',
        'libjingle/xmpp/pubsubtasks_unittest.cc',
        'libjingle/xmpp/util_unittest.cc',
        'libjingle/xmpp/util_unittest.h',
        'libjingle/xmpp/xmppengine_unittest.cc',
        'libjingle/xmpp/xmpplogintask_unittest.cc',
        'libjingle/xmpp/xmppstanzaparser_unittest.cc',
      ],
    },
    {
      'target_name': 'webrtc_tests',
      'type': 'none',
      'dependencies': [
        'video_engine_tests',
        'video_loopback',
        'video_replay',
        'webrtc_perf_tests',
        'webrtc_nonparallel_tests',
      ],
    },
    {
      'target_name': 'video_quality_test',
      'type': 'static_library',
      'sources': [
        'video/video_quality_test.cc',
        'video/video_quality_test.h',
      ],
      'dependencies': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(webrtc_root)/modules/modules.gyp:video_capture_module_internal_impl',
        '<(webrtc_root)/system_wrappers/system_wrappers.gyp:system_wrappers',
        'webrtc',
      ],
      'conditions': [
        ['OS=="android"', {
          'dependencies!': [
            '<(webrtc_root)/modules/modules.gyp:video_capture_module_internal_impl',
          ],
        }],
      ],
    },
    {
      'target_name': 'video_loopback',
      'type': 'executable',
      'sources': [
        'test/mac/run_test.mm',
        'test/run_test.cc',
        'test/run_test.h',
        'video/video_loopback.cc',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources!': [
            'test/run_test.cc',
          ],
        }],
      ],
      'dependencies': [
        'video_quality_test',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/gflags/gflags.gyp:gflags',
        'test/test.gyp:test_common',
        'test/test.gyp:test_main',
        'test/test.gyp:test_renderer',
        'webrtc',
      ],
    },
    {
      'target_name': 'screenshare_loopback',
      'type': 'executable',
      'sources': [
        'test/mac/run_test.mm',
        'test/run_test.cc',
        'test/run_test.h',
        'video/screenshare_loopback.cc',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources!': [
            'test/run_test.cc',
          ],
        }],
      ],
      'dependencies': [
        'video_quality_test',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/gflags/gflags.gyp:gflags',
        'test/test.gyp:test_common',
        'test/test.gyp:test_main',
        'test/test.gyp:test_renderer',
        'webrtc',
      ],
    },
    {
      'target_name': 'video_replay',
      'type': 'executable',
      'sources': [
        'test/mac/run_test.mm',
        'test/run_test.cc',
        'test/run_test.h',
        'video/replay.cc',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources!': [
            'test/run_test.cc',
          ],
        }],
      ],
      'dependencies': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/gflags/gflags.gyp:gflags',
        'test/test.gyp:test_common',
        'test/test.gyp:test_renderer',
        '<(webrtc_root)/modules/modules.gyp:video_capture',
        '<(webrtc_root)/system_wrappers/system_wrappers.gyp:system_wrappers_default',
        'webrtc',
      ],
    },
    {
      # TODO(solenberg): Rename to webrtc_call_tests.
      'target_name': 'video_engine_tests',
      'type': '<(gtest_target_type)',
      'sources': [
        'audio/audio_receive_stream_unittest.cc',
        'audio/audio_send_stream_unittest.cc',
        'audio/audio_state_unittest.cc',
        'call/bitrate_allocator_unittest.cc',
        'call/bitrate_estimator_tests.cc',
        'call/call_unittest.cc',
        'call/packet_injection_tests.cc',
        'call/ringbuffer_unittest.cc',
        'test/common_unittest.cc',
        'test/testsupport/metrics/video_metrics_unittest.cc',
        'video/call_stats_unittest.cc',
        'video/encoder_state_feedback_unittest.cc',
        'video/end_to_end_tests.cc',
        'video/overuse_frame_detector_unittest.cc',
        'video/payload_router_unittest.cc',
        'video/report_block_stats_unittest.cc',
        'video/send_delay_stats_unittest.cc',
        'video/send_statistics_proxy_unittest.cc',
        'video/stats_counter_unittest.cc',
        'video/stream_synchronization_unittest.cc',
        'video/video_capture_input_unittest.cc',
        'video/video_decoder_unittest.cc',
        'video/video_encoder_unittest.cc',
        'video/video_send_stream_tests.cc',
        'video/vie_remb_unittest.cc',
      ],
      'dependencies': [
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(webrtc_root)/common.gyp:webrtc_common',
        '<(webrtc_root)/modules/modules.gyp:rtp_rtcp',
        '<(webrtc_root)/modules/modules.gyp:video_capture',
        '<(webrtc_root)/test/test.gyp:channel_transport',
        '<(webrtc_root)/voice_engine/voice_engine.gyp:voice_engine',
        'test/metrics.gyp:metrics',
        'test/test.gyp:test_common',
        'test/test.gyp:test_main',
        'webrtc',
      ],
      'conditions': [
        ['rtc_use_h264==1', {
          'defines': [
            'WEBRTC_END_TO_END_H264_TESTS',
          ],
        }],
        ['OS=="android"', {
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['OS=="ios"', {
          'mac_bundle_resources': [
            '<(DEPTH)/resources/foreman_cif_short.yuv',
            '<(DEPTH)/resources/voice_engine/audio_long16.pcm',
          ],
        }],
        ['enable_protobuf==1', {
          'defines': [
            'ENABLE_RTC_EVENT_LOG',
          ],
          'dependencies': [
            'webrtc.gyp:rtc_event_log',
            'webrtc.gyp:rtc_event_log_parser',
            'webrtc.gyp:rtc_event_log_proto',
          ],
          'sources': [
            'call/rtc_event_log_unittest.cc',
            'call/rtc_event_log_unittest_helper.cc'
          ],
        }],
      ],
    },
    {
      'target_name': 'webrtc_perf_tests',
      'type': '<(gtest_target_type)',
      'sources': [
        'call/call_perf_tests.cc',
        'call/rampup_tests.cc',
        'call/rampup_tests.h',
        'modules/audio_coding/neteq/test/neteq_performance_unittest.cc',
        'modules/audio_processing/audio_processing_performance_unittest.cc',
        'modules/remote_bitrate_estimator/remote_bitrate_estimators_test.cc',
        'video/full_stack.cc',
      ],
      'dependencies': [
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(webrtc_root)/modules/modules.gyp:audio_processing',
        '<(webrtc_root)/modules/modules.gyp:audioproc_test_utils',
        '<(webrtc_root)/modules/modules.gyp:video_capture',
        '<(webrtc_root)/test/test.gyp:channel_transport',
        '<(webrtc_root)/voice_engine/voice_engine.gyp:voice_engine',
        'video_quality_test',
        'modules/modules.gyp:neteq_test_support',
        'modules/modules.gyp:bwe_simulator',
        'modules/modules.gyp:rtp_rtcp',
        'test/test.gyp:test_common',
        'test/test.gyp:test_main',
        'test/test.gyp:test_renderer',
        'webrtc',
      ],
      'conditions': [
        ['OS=="android"', {
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
      ],
    },
    {
      'target_name': 'webrtc_nonparallel_tests',
      'type': '<(gtest_target_type)',
      'sources': [
        'base/nullsocketserver_unittest.cc',
        'base/physicalsocketserver_unittest.cc',
        'base/socket_unittest.cc',
        'base/socket_unittest.h',
        'base/socketaddress_unittest.cc',
        'base/virtualsocket_unittest.cc',
      ],
      'defines': [
        'GTEST_RELATIVE_PATH',
      ],
      'dependencies': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'base/base.gyp:rtc_base',
        'test/test.gyp:test_main',
      ],
      'conditions': [
        ['OS=="android"', {
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'base/win32socketserver_unittest.cc',
          ],
          'sources!': [
            # TODO(ronghuawu): Fix TestUdpReadyToSendIPv6 on windows bot
            # then reenable these tests.
            # TODO(pbos): Move test disabling to ifdefs within the test files
            # instead of here.
            'base/physicalsocketserver_unittest.cc',
            'base/socket_unittest.cc',
            'base/win32socketserver_unittest.cc',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            'base/macsocketserver_unittest.cc',
          ],
        }],
        ['OS=="ios" or (OS=="mac" and target_arch!="ia32")', {
          'defines': [
            'CARBON_DEPRECATED=YES',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'rtc_unittests_apk_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):rtc_unittests_apk',
          ],
        },
        {
          'target_name': 'video_engine_tests_apk_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):video_engine_tests_apk',
          ],
        },
        {
          'target_name': 'webrtc_perf_tests_apk_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):webrtc_perf_tests_apk',
          ],
        },
        {
          'target_name': 'webrtc_nonparallel_tests_apk_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):webrtc_nonparallel_tests_apk',
          ],
        },
        {
          'target_name': 'android_junit_tests_target',
          'type': 'none',
          'dependencies': [
            '<(android_tests_path):android_junit_tests',
          ],
        },
      ],
      'conditions': [
        ['test_isolation_mode != "noop"',
          {
            'targets': [
              {
                'target_name': 'rtc_unittests_apk_run',
                'type': 'none',
                'dependencies': [
                  '<(android_tests_path):rtc_unittests_apk',
                ],
                'includes': [
                  'build/isolate.gypi',
                ],
                'sources': [
                  'rtc_unittests_apk.isolate',
                ],
              },
              {
                'target_name': 'video_engine_tests_apk_run',
                'type': 'none',
                'dependencies': [
                  '<(android_tests_path):video_engine_tests_apk',
                ],
                'includes': [
                  'build/isolate.gypi',
                ],
                'sources': [
                  'video_engine_tests_apk.isolate',
                ],
              },
              {
                'target_name': 'webrtc_perf_tests_apk_run',
                'type': 'none',
                'dependencies': [
                  '<(android_tests_path):webrtc_perf_tests_apk',
                ],
                'includes': [
                  'build/isolate.gypi',
                ],
                'sources': [
                  'webrtc_perf_tests_apk.isolate',
                ],
              },
              {
                'target_name': 'webrtc_nonparallel_tests_apk_run',
                'type': 'none',
                'dependencies': [
                  '<(android_tests_path):webrtc_nonparallel_tests_apk',
                ],
                'includes': [
                  'build/isolate.gypi',
                ],
                'sources': [
                  'webrtc_nonparallel_tests_apk.isolate',
                ],
              },
            ],
          },
        ],
      ],
    }],
    ['test_isolation_mode != "noop"', {
      'targets': [
        {
          'target_name': 'rtc_unittests_run',
          'type': 'none',
          'dependencies': [
            'rtc_unittests',
          ],
          'includes': [
            'build/isolate.gypi',
          ],
          'sources': [
            'rtc_unittests.isolate',
          ],
        },
        {
          'target_name': 'video_engine_tests_run',
          'type': 'none',
          'dependencies': [
            'video_engine_tests',
          ],
          'includes': [
            'build/isolate.gypi',
          ],
          'sources': [
            'video_engine_tests.isolate',
          ],
        },
        {
          'target_name': 'webrtc_nonparallel_tests_run',
          'type': 'none',
          'dependencies': [
            'webrtc_nonparallel_tests',
          ],
          'includes': [
            'build/isolate.gypi',
          ],
          'sources': [
            'webrtc_nonparallel_tests.isolate',
          ],
        },
        {
          'target_name': 'webrtc_perf_tests_run',
          'type': 'none',
          'dependencies': [
            'webrtc_perf_tests',
          ],
          'includes': [
            'build/isolate.gypi',
          ],
          'sources': [
            'webrtc_perf_tests.isolate',
          ],
        },
      ],
    }],
  ],
}
