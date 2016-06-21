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

#import "RTCSessionDescriptionSyncObserver.h"

#import "RTCSessionDescription.h"

@interface RTCSessionDescriptionSyncObserver ()

// CondVar used to wait for, and signal arrival of, an SDP-related callback.
@property(nonatomic, strong) NSCondition* condition;
// Whether an SDP-related callback has fired; cleared before wait returns.
@property(atomic, assign) BOOL signaled;

@end

@implementation RTCSessionDescriptionSyncObserver

@synthesize error = _error;
@synthesize sessionDescription = _sessionDescription;
@synthesize success = _success;
@synthesize condition = _condition;
@synthesize signaled = _signaled;

- (id)init {
  if ((self = [super init])) {
    if (!(_condition = [[NSCondition alloc] init]))
      self = nil;
  }
  return self;
}

- (void)signal {
  self.signaled = YES;
  [self.condition signal];
}

- (void)wait {
  [self.condition lock];
  if (!self.signaled)
    [self.condition wait];
  self.signaled = NO;
  [self.condition unlock];
}

#pragma mark - RTCSessionDescriptionDelegate methods
- (void)peerConnection:(RTCPeerConnection*)peerConnection
    didCreateSessionDescription:(RTCSessionDescription*)sdp
                          error:(NSError*)error {
  [self.condition lock];
  if (error) {
    self.success = NO;
    self.error = error.description;
  } else {
    self.success = YES;
    self.sessionDescription = sdp;
  }
  [self signal];
  [self.condition unlock];
}

- (void)peerConnection:(RTCPeerConnection*)peerConnection
    didSetSessionDescriptionWithError:(NSError*)error {
  [self.condition lock];
  if (error) {
    self.success = NO;
    self.error = error.description;
  } else {
    self.success = YES;
  }
  [self signal];
  [self.condition unlock];
}

@end
