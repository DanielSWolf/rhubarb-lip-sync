/*
 * libjingle
 * Copyright 2015 Google Inc.
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

#import "RTCLogging.h"

#include "webrtc/base/logging.h"

rtc::LoggingSeverity RTCGetNativeLoggingSeverity(RTCLoggingSeverity severity) {
  switch (severity) {
      case kRTCLoggingSeverityVerbose:
        return rtc::LS_VERBOSE;
      case kRTCLoggingSeverityInfo:
        return rtc::LS_INFO;
      case kRTCLoggingSeverityWarning:
        return rtc::LS_WARNING;
      case kRTCLoggingSeverityError:
        return rtc::LS_ERROR;
  }
}

void RTCLogEx(RTCLoggingSeverity severity, NSString* logString) {
  if (logString.length) {
    const char* utf8String = logString.UTF8String;
    LOG_V(RTCGetNativeLoggingSeverity(severity)) << utf8String;
  }
}

void RTCSetMinDebugLogLevel(RTCLoggingSeverity severity) {
  rtc::LogMessage::LogToDebug(RTCGetNativeLoggingSeverity(severity));
}

NSString* RTCFileName(const char* filePath) {
  NSString* nsFilePath =
      [[NSString alloc] initWithBytesNoCopy:const_cast<char*>(filePath)
                                     length:strlen(filePath)
                                   encoding:NSUTF8StringEncoding
                               freeWhenDone:NO];
  return nsFilePath.lastPathComponent;
}

