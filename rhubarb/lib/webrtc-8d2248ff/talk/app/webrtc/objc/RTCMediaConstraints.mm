/*
 * libjingle
 * Copyright 2013 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCMediaConstraints+Internal.h"

#import "RTCPair.h"

#include <memory>

// TODO(hughv):  Add accessors for mandatory and optional constraints.
// TODO(hughv):  Add description.

@implementation RTCMediaConstraints {
  std::unique_ptr<webrtc::RTCMediaConstraintsNative> _constraints;
  webrtc::MediaConstraintsInterface::Constraints _mandatory;
  webrtc::MediaConstraintsInterface::Constraints _optional;
}

- (id)initWithMandatoryConstraints:(NSArray*)mandatory
               optionalConstraints:(NSArray*)optional {
  if ((self = [super init])) {
    _mandatory = [[self class] constraintsFromArray:mandatory];
    _optional = [[self class] constraintsFromArray:optional];
    _constraints.reset(
        new webrtc::RTCMediaConstraintsNative(_mandatory, _optional));
  }
  return self;
}

+ (webrtc::MediaConstraintsInterface::Constraints)constraintsFromArray:
                                                      (NSArray*)array {
  webrtc::MediaConstraintsInterface::Constraints constraints;
  for (RTCPair* pair in array) {
    constraints.push_back(webrtc::MediaConstraintsInterface::Constraint(
        [pair.key UTF8String], [pair.value UTF8String]));
  }
  return constraints;
}

@end

@implementation RTCMediaConstraints (internal)

- (const webrtc::RTCMediaConstraintsNative*)constraints {
  return _constraints.get();
}

@end
