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

#import "RTCNSGLVideoView.h"

#import <CoreVideo/CVDisplayLink.h>
#import <OpenGL/gl3.h>
#import "RTCI420Frame.h"
#import "RTCOpenGLVideoRenderer.h"

@interface RTCNSGLVideoView ()
// |i420Frame| is set when we receive a frame from a worker thread and is read
// from the display link callback so atomicity is required.
@property(atomic, strong) RTCI420Frame* i420Frame;
@property(atomic, strong) RTCOpenGLVideoRenderer* glRenderer;
- (void)drawFrame;
@end

static CVReturn OnDisplayLinkFired(CVDisplayLinkRef displayLink,
                                   const CVTimeStamp* now,
                                   const CVTimeStamp* outputTime,
                                   CVOptionFlags flagsIn,
                                   CVOptionFlags* flagsOut,
                                   void* displayLinkContext) {
  RTCNSGLVideoView* view = (__bridge RTCNSGLVideoView*)displayLinkContext;
  [view drawFrame];
  return kCVReturnSuccess;
}

@implementation RTCNSGLVideoView {
  CVDisplayLinkRef _displayLink;
}

- (void)dealloc {
  [self teardownDisplayLink];
}

- (void)drawRect:(NSRect)rect {
  [self drawFrame];
}

- (void)reshape {
  [super reshape];
  NSRect frame = [self frame];
  CGLLockContext([[self openGLContext] CGLContextObj]);
  glViewport(0, 0, frame.size.width, frame.size.height);
  CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)lockFocus {
  NSOpenGLContext* context = [self openGLContext];
  [super lockFocus];
  if ([context view] != self) {
    [context setView:self];
  }
  [context makeCurrentContext];
}

- (void)prepareOpenGL {
  [super prepareOpenGL];
  if (!self.glRenderer) {
    self.glRenderer =
        [[RTCOpenGLVideoRenderer alloc] initWithContext:[self openGLContext]];
  }
  [self.glRenderer setupGL];
  [self setupDisplayLink];
}

- (void)clearGLContext {
  [self.glRenderer teardownGL];
  self.glRenderer = nil;
  [super clearGLContext];
}

#pragma mark - RTCVideoRenderer

// These methods may be called on non-main thread.
- (void)setSize:(CGSize)size {
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.delegate videoView:self didChangeVideoSize:size];
  });
}

- (void)renderFrame:(RTCI420Frame*)frame {
  self.i420Frame = frame;
}

#pragma mark - Private

- (void)drawFrame {
  RTCI420Frame* i420Frame = self.i420Frame;
  if (self.glRenderer.lastDrawnFrame != i420Frame) {
    // This method may be called from CVDisplayLink callback which isn't on the
    // main thread so we have to lock the GL context before drawing.
    CGLLockContext([[self openGLContext] CGLContextObj]);
    [self.glRenderer drawFrame:i420Frame];
    CGLUnlockContext([[self openGLContext] CGLContextObj]);
  }
}

- (void)setupDisplayLink {
  if (_displayLink) {
    return;
  }
  // Synchronize buffer swaps with vertical refresh rate.
  GLint swapInt = 1;
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  // Create display link.
  CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
  CVDisplayLinkSetOutputCallback(_displayLink,
                                 &OnDisplayLinkFired,
                                 (__bridge void*)self);
  // Set the display link for the current renderer.
  CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
  CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
  CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(
      _displayLink, cglContext, cglPixelFormat);
  CVDisplayLinkStart(_displayLink);
}

- (void)teardownDisplayLink {
  if (!_displayLink) {
    return;
  }
  CVDisplayLinkRelease(_displayLink);
  _displayLink = NULL;
}

@end
