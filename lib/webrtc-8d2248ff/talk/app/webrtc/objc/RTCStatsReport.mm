/*
 * libjingle
 * Copyright 2014 Google Inc.
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

#import "RTCStatsReport+Internal.h"

#import "RTCPair.h"

@implementation RTCStatsReport

- (NSString*)description {
  NSString* format = @"id: %@, type: %@, timestamp: %f, values: %@";
  return [NSString stringWithFormat:format,
                                    self.reportId,
                                    self.type,
                                    self.timestamp,
                                    self.values];
}

@end

@implementation RTCStatsReport (Internal)

- (instancetype)initWithStatsReport:(const webrtc::StatsReport&)statsReport {
  if (self = [super init]) {
    _reportId = @(statsReport.id()->ToString().c_str());
    _type = @(statsReport.TypeToString());
    _timestamp = statsReport.timestamp();
    NSMutableArray* values =
        [NSMutableArray arrayWithCapacity:statsReport.values().size()];
    for (const auto& it : statsReport.values()) {
      RTCPair* pair =
          [[RTCPair alloc] initWithKey:@(it.second->display_name())
                                 value:@(it.second->ToString().c_str())];
      [values addObject:pair];
    }
    _values = values;
  }
  return self;
}

@end
