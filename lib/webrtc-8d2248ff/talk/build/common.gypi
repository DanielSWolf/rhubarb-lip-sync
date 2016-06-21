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
#

# This file contains common settings for building libjingle components.

{
  'variables': {
    'webrtc_root%': '<(DEPTH)/webrtc',
    # TODO(ronghuawu): For now, disable the Chrome plugins, which causes a
    # flood of chromium-style warnings.
    'clang_use_chrome_plugins%': 0,
    # Disable these to not build components which can be externally provided.
    'build_json%': 1,
  },
  'target_defaults': {
    'include_dirs': [
      '<(DEPTH)',
      '../..',
      '../../third_party',
      '../../third_party/webrtc',
      '../../webrtc',
    ],
    'conditions': [
      ['OS=="linux"', {
        'defines': [
          'WEBRTC_LINUX',
        ],
        # Remove Chromium's disabling of the -Wformat warning.
        'cflags!': [
          '-Wno-format',
        ],
        'conditions': [
          ['clang==1', {
            'cflags': [
              '-Wall',
              '-Wextra',
              '-Wformat',
              '-Wformat-security',
              '-Wimplicit-fallthrough',
              '-Wmissing-braces',
              '-Wreorder',
              '-Wunused-variable',
              # TODO(ronghuawu): Fix the warning caused by
              # LateBindingSymbolTable::TableInfo from
              # latebindingsymboltable.cc.def and remove below flag.
              '-Wno-address-of-array-temporary',
              '-Wthread-safety',
            ],
            'cflags_cc': [
              '-Wunused-private-field',
            ],
          }],
        ],
      }],
      ['OS=="mac"', {
        'defines': [
          'WEBRTC_MAC',
        ],
      }],
      ['OS=="win"', {
        'defines': [
          'WEBRTC_WIN',
        ],
        'msvs_disabled_warnings': [
          # https://code.google.com/p/chromium/issues/detail?id=372451#c20
          # Warning 4702 ("Unreachable code") should be re-enabled once
          # users are updated to VS2013 Update 2.
            4702,
        ],
      }],
      ['OS=="ios"', {
        'defines': [
          'WEBRTC_MAC',
          'WEBRTC_IOS',
        ],
      }],
      ['OS=="ios" or (OS=="mac" and target_arch!="ia32")', {
        'defines': [
          'CARBON_DEPRECATED=YES',
        ],
      }],
      ['os_posix==1', {
        'configurations': {
          'Debug_Base': {
            'defines': [
              # Chromium's build/common.gypi defines this for all posix _except_
              # for ios & mac.  We want it there as well, e.g. because ASSERT
              # and friends trigger off of it.
              '_DEBUG',
            ],
          },
        },
        'defines': [
          'HASH_NAMESPACE=__gnu_cxx',
          'WEBRTC_POSIX',
          'DISABLE_DYNAMIC_CAST',
          # The POSIX standard says we have to define this.
          '_REENTRANT',
        ],
      }],
    ],
  }, # target_defaults
}
