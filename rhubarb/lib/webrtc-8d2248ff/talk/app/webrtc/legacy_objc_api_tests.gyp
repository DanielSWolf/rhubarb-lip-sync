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
      # The >=10.7 above is required to make ARC link cleanly (e.g. as
      # opposed to _compile_ cleanly, which the library under test
      # does just fine on 10.6 too).
      'targets': [
        {
          'target_name': 'libjingle_peerconnection_objc_test',
          'type': 'executable',
          'includes': [ '../../../webrtc/build/ios/objc_app.gypi' ],
          'dependencies': [
            '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:field_trial_default',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
            'legacy_objc_api.gyp:libjingle_peerconnection_objc',
          ],
          'sources': [
            'objctests/RTCPeerConnectionSyncObserver.h',
            'objctests/RTCPeerConnectionSyncObserver.m',
            'objctests/RTCPeerConnectionTest.mm',
            'objctests/RTCSessionDescriptionSyncObserver.h',
            'objctests/RTCSessionDescriptionSyncObserver.m',
            # TODO(fischman): figure out if this works for ios or if it
            # needs a GUI driver.
            'objctests/mac/main.mm',
          ],
          'conditions': [
            ['OS=="mac"', {
              'xcode_settings': {
                # Need to build against 10.7 framework for full ARC support
                # on OSX.
                'MACOSX_DEPLOYMENT_TARGET' : '10.7',
                # common.gypi enables this for mac but we want this to be
                # disabled like it is for ios.
                'CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS': 'NO',
              },
            }],
          ],
        },  # target libjingle_peerconnection_objc_test
        {
          'target_name': 'apprtc_signaling_gunit_test',
          'type': 'executable',
          'includes': [ '../../../webrtc/build/ios/objc_app.gypi' ],
          'dependencies': [
            '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:field_trial_default',
            '<(webrtc_root)/system_wrappers/system_wrappers.gyp:metrics_default',
            '<(webrtc_root)/webrtc_examples.gyp:apprtc_signaling',
            '<(DEPTH)/third_party/ocmock/ocmock.gyp:ocmock',
          ],
          'sources': [
            'objctests/mac/main.mm',
            '<(webrtc_root)/examples/objc/AppRTCDemo/tests/ARDAppClientTest.mm',
          ],
          'conditions': [
            ['OS=="mac"', {
              'xcode_settings': {
                'MACOSX_DEPLOYMENT_TARGET' : '10.8',
              },
            }],
          ],
        },  # target apprtc_signaling_gunit_test
      ],
    }],
  ],
}
