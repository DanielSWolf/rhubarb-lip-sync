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

#import "RTCEAGLVideoView.h"

#import <GLKit/GLKit.h>

#import "RTCI420Frame.h"
#import "RTCOpenGLVideoRenderer.h"

// RTCDisplayLinkTimer wraps a CADisplayLink and is set to fire every two screen
// refreshes, which should be 30fps. We wrap the display link in order to avoid
// a retain cycle since CADisplayLink takes a strong reference onto its target.
// The timer is paused by default.
@interface RTCDisplayLinkTimer : NSObject

@property(nonatomic) BOOL isPaused;

- (instancetype)initWithTimerHandler:(void (^)(void))timerHandler;
- (void)invalidate;

@end

@implementation RTCDisplayLinkTimer {
  CADisplayLink* _displayLink;
  void (^_timerHandler)(void);
}

- (instancetype)initWithTimerHandler:(void (^)(void))timerHandler {
  NSParameterAssert(timerHandler);
  if (self = [super init]) {
    _timerHandler = timerHandler;
    _displayLink =
        [CADisplayLink displayLinkWithTarget:self
                                    selector:@selector(displayLinkDidFire:)];
    _displayLink.paused = YES;
    // Set to half of screen refresh, which should be 30fps.
    [_displayLink setFrameInterval:2];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop]
                       forMode:NSRunLoopCommonModes];
  }
  return self;
}

- (void)dealloc {
  [self invalidate];
}

- (BOOL)isPaused {
  return _displayLink.paused;
}

- (void)setIsPaused:(BOOL)isPaused {
  _displayLink.paused = isPaused;
}

- (void)invalidate {
  [_displayLink invalidate];
}

- (void)displayLinkDidFire:(CADisplayLink*)displayLink {
  _timerHandler();
}

@end

// RTCEAGLVideoView wraps a GLKView which is setup with
// enableSetNeedsDisplay = NO for the purpose of gaining control of
// exactly when to call -[GLKView display]. This need for extra
// control is required to avoid triggering method calls on GLKView
// that results in attempting to bind the underlying render buffer
// when the drawable size would be empty which would result in the
// error GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT. -[GLKView display] is
// the method that will trigger the binding of the render
// buffer. Because the standard behaviour of -[UIView setNeedsDisplay]
// is disabled for the reasons above, the RTCEAGLVideoView maintains
// its own |isDirty| flag.

@interface RTCEAGLVideoView () <GLKViewDelegate>
// |i420Frame| is set when we receive a frame from a worker thread and is read
// from the display link callback so atomicity is required.
@property(atomic, strong) RTCI420Frame* i420Frame;
@property(nonatomic, readonly) GLKView* glkView;
@property(nonatomic, readonly) RTCOpenGLVideoRenderer* glRenderer;
@end

@implementation RTCEAGLVideoView {
  RTCDisplayLinkTimer* _timer;
  GLKView* _glkView;
  RTCOpenGLVideoRenderer* _glRenderer;
  // This flag should only be set and read on the main thread (e.g. by
  // setNeedsDisplay)
  BOOL _isDirty;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self configure];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  if (self = [super initWithCoder:aDecoder]) {
    [self configure];
  }
  return self;
}

- (void)configure {
  EAGLContext* glContext =
    [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!glContext) {
    glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  }
  _glRenderer = [[RTCOpenGLVideoRenderer alloc] initWithContext:glContext];

  // GLKView manages a framebuffer for us.
  _glkView = [[GLKView alloc] initWithFrame:CGRectZero
                                    context:glContext];
  _glkView.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
  _glkView.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
  _glkView.drawableStencilFormat = GLKViewDrawableStencilFormatNone;
  _glkView.drawableMultisample = GLKViewDrawableMultisampleNone;
  _glkView.delegate = self;
  _glkView.layer.masksToBounds = YES;
  _glkView.enableSetNeedsDisplay = NO;
  [self addSubview:_glkView];

  // Listen to application state in order to clean up OpenGL before app goes
  // away.
  NSNotificationCenter* notificationCenter =
    [NSNotificationCenter defaultCenter];
  [notificationCenter addObserver:self
                         selector:@selector(willResignActive)
                             name:UIApplicationWillResignActiveNotification
                           object:nil];
  [notificationCenter addObserver:self
                         selector:@selector(didBecomeActive)
                             name:UIApplicationDidBecomeActiveNotification
                           object:nil];

  // Frames are received on a separate thread, so we poll for current frame
  // using a refresh rate proportional to screen refresh frequency. This
  // occurs on the main thread.
  __weak RTCEAGLVideoView* weakSelf = self;
  _timer = [[RTCDisplayLinkTimer alloc] initWithTimerHandler:^{
      RTCEAGLVideoView* strongSelf = weakSelf;
      [strongSelf displayLinkTimerDidFire];
    }];
  [self setupGL];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  UIApplicationState appState =
      [UIApplication sharedApplication].applicationState;
  if (appState == UIApplicationStateActive) {
    [self teardownGL];
  }
  [_timer invalidate];
}

#pragma mark - UIView

- (void)setNeedsDisplay {
  [super setNeedsDisplay];
  _isDirty = YES;
}

- (void)setNeedsDisplayInRect:(CGRect)rect {
  [super setNeedsDisplayInRect:rect];
  _isDirty = YES;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _glkView.frame = self.bounds;
}

#pragma mark - GLKViewDelegate

// This method is called when the GLKView's content is dirty and needs to be
// redrawn. This occurs on main thread.
- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect {
  // The renderer will draw the frame to the framebuffer corresponding to the
  // one used by |view|.
  [_glRenderer drawFrame:self.i420Frame];
}

#pragma mark - RTCVideoRenderer

// These methods may be called on non-main thread.
- (void)setSize:(CGSize)size {
  __weak RTCEAGLVideoView* weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    RTCEAGLVideoView* strongSelf = weakSelf;
    [strongSelf.delegate videoView:strongSelf didChangeVideoSize:size];
  });
}

- (void)renderFrame:(RTCI420Frame*)frame {
  self.i420Frame = frame;
}

#pragma mark - Private

- (void)displayLinkTimerDidFire {
  // Don't render unless video frame have changed or the view content
  // has explicitly been marked dirty.
  if (!_isDirty && _glRenderer.lastDrawnFrame == self.i420Frame) {
    return;
  }

  // Always reset isDirty at this point, even if -[GLKView display]
  // won't be called in the case the drawable size is empty.
  _isDirty = NO;

  // Only call -[GLKView display] if the drawable size is
  // non-empty. Calling display will make the GLKView setup its
  // render buffer if necessary, but that will fail with error
  // GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT if size is empty.
  if (self.bounds.size.width > 0 && self.bounds.size.height > 0) {
    [_glkView display];
  }
}

- (void)setupGL {
  self.i420Frame = nil;
  [_glRenderer setupGL];
  _timer.isPaused = NO;
}

- (void)teardownGL {
  self.i420Frame = nil;
  _timer.isPaused = YES;
  [_glkView deleteDrawable];
  [_glRenderer teardownGL];
}

- (void)didBecomeActive {
  [self setupGL];
}

- (void)willResignActive {
  [self teardownGL];
}

@end
