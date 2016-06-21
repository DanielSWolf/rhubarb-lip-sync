/*
 *  Copyright 2014 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "APPRTCViewController.h"

#import <AVFoundation/AVFoundation.h>

#import "WebRTC/RTCNSGLVideoView.h"
#import "WebRTC/RTCVideoTrack.h"

#import "ARDAppClient.h"

static NSUInteger const kContentWidth = 1280;
static NSUInteger const kContentHeight = 720;
static NSUInteger const kRoomFieldWidth = 80;
static NSUInteger const kLogViewHeight = 280;

@class APPRTCMainView;
@protocol APPRTCMainViewDelegate

- (void)appRTCMainView:(APPRTCMainView*)mainView
        didEnterRoomId:(NSString*)roomId;

@end

@interface APPRTCMainView : NSView

@property(nonatomic, weak) id<APPRTCMainViewDelegate> delegate;
@property(nonatomic, readonly) RTCNSGLVideoView* localVideoView;
@property(nonatomic, readonly) RTCNSGLVideoView* remoteVideoView;

- (void)displayLogMessage:(NSString*)message;

@end

@interface APPRTCMainView () <NSTextFieldDelegate, RTCNSGLVideoViewDelegate>
@end
@implementation APPRTCMainView  {
  NSScrollView* _scrollView;
  NSTextField* _roomLabel;
  NSTextField* _roomField;
  NSTextView* _logView;
  RTCNSGLVideoView* _localVideoView;
  RTCNSGLVideoView* _remoteVideoView;
  CGSize _localVideoSize;
  CGSize _remoteVideoSize;
}

+ (BOOL)requiresConstraintBasedLayout {
  return YES;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self setupViews];
  }
  return self;
}

- (void)updateConstraints {
  NSParameterAssert(
      _roomField != nil && _scrollView != nil && _remoteVideoView != nil);
  [self removeConstraints:[self constraints]];
  NSDictionary* viewsDictionary =
      NSDictionaryOfVariableBindings(_roomLabel,
                                     _roomField,
                                     _scrollView,
                                     _remoteVideoView);

  NSSize remoteViewSize = [self remoteVideoViewSize];
  NSDictionary* metrics = @{
    @"kLogViewHeight" : @(kLogViewHeight),
    @"kRoomFieldWidth" : @(kRoomFieldWidth),
    @"remoteViewWidth" : @(remoteViewSize.width),
    @"remoteViewHeight" : @(remoteViewSize.height),
  };
  // Declare this separately to avoid compiler warning about splitting string
  // within an NSArray expression.
  NSString* verticalConstraint =
      @"V:|-[_roomLabel]-[_roomField]-[_scrollView(kLogViewHeight)]"
       "-[_remoteVideoView(remoteViewHeight)]-|";
  NSArray* constraintFormats = @[
      verticalConstraint,
      @"|-[_roomLabel]",
      @"|-[_roomField(kRoomFieldWidth)]",
      @"|-[_scrollView(remoteViewWidth)]-|",
      @"|-[_remoteVideoView(remoteViewWidth)]-|",
  ];
  for (NSString* constraintFormat in constraintFormats) {
    NSArray* constraints =
        [NSLayoutConstraint constraintsWithVisualFormat:constraintFormat
                                                options:0
                                                metrics:metrics
                                                  views:viewsDictionary];
    for (NSLayoutConstraint* constraint in constraints) {
      [self addConstraint:constraint];
    }
  }
  [super updateConstraints];
}

- (void)displayLogMessage:(NSString*)message {
  _logView.string =
      [NSString stringWithFormat:@"%@%@\n", _logView.string, message];
  NSRange range = NSMakeRange([_logView.string length], 0);
  [_logView scrollRangeToVisible:range];
}

#pragma mark - NSControl delegate

- (void)controlTextDidEndEditing:(NSNotification*)notification {
  NSDictionary* userInfo = [notification userInfo];
  NSInteger textMovement = [userInfo[@"NSTextMovement"] intValue];
  if (textMovement == NSReturnTextMovement) {
    [self.delegate appRTCMainView:self didEnterRoomId:_roomField.stringValue];
  }
}

#pragma mark - RTCNSGLVideoViewDelegate

- (void)videoView:(RTCNSGLVideoView*)videoView
    didChangeVideoSize:(NSSize)size {
  if (videoView == _remoteVideoView) {
    _remoteVideoSize = size;
  } else if (videoView == _localVideoView) {
    _localVideoSize = size;
  } else {
    return;
  }
  [self setNeedsUpdateConstraints:YES];
}

#pragma mark - Private

- (void)setupViews {
  NSParameterAssert([[self subviews] count] == 0);

  _roomLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  [_roomLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_roomLabel setBezeled:NO];
  [_roomLabel setDrawsBackground:NO];
  [_roomLabel setEditable:NO];
  [_roomLabel setStringValue:@"Enter AppRTC room id:"];
  [self addSubview:_roomLabel];

  _roomField = [[NSTextField alloc] initWithFrame:NSZeroRect];
  [_roomField setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self addSubview:_roomField];
  [_roomField setEditable:YES];
  [_roomField setDelegate:self];

  _logView = [[NSTextView alloc] initWithFrame:NSZeroRect];
  [_logView setMinSize:NSMakeSize(0, kLogViewHeight)];
  [_logView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
  [_logView setVerticallyResizable:YES];
  [_logView setAutoresizingMask:NSViewWidthSizable];
  NSTextContainer* textContainer = [_logView textContainer];
  NSSize containerSize = NSMakeSize(kContentWidth, FLT_MAX);
  [textContainer setContainerSize:containerSize];
  [textContainer setWidthTracksTextView:YES];
  [_logView setEditable:NO];

  _scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
  [_scrollView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_scrollView setHasVerticalScroller:YES];
  [_scrollView setDocumentView:_logView];
  [self addSubview:_scrollView];

  NSOpenGLPixelFormatAttribute attributes[] = {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFADepthSize, 24,
    NSOpenGLPFAOpenGLProfile,
    NSOpenGLProfileVersion3_2Core,
    0
  };
  NSOpenGLPixelFormat* pixelFormat =
      [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  _remoteVideoView = [[RTCNSGLVideoView alloc] initWithFrame:NSZeroRect
                                                 pixelFormat:pixelFormat];
  [_remoteVideoView setTranslatesAutoresizingMaskIntoConstraints:NO];
  _remoteVideoView.delegate = self;
  [self addSubview:_remoteVideoView];

  // TODO(tkchin): create local video view.
  // https://code.google.com/p/webrtc/issues/detail?id=3417.
}

- (NSSize)remoteVideoViewSize {
  if (_remoteVideoSize.width > 0 && _remoteVideoSize.height > 0) {
    return _remoteVideoSize;
  } else {
    return NSMakeSize(kContentWidth, kContentHeight);
  }
}

- (NSSize)localVideoViewSize {
  return NSZeroSize;
}

@end

@interface APPRTCViewController ()
    <ARDAppClientDelegate, APPRTCMainViewDelegate>
@property(nonatomic, readonly) APPRTCMainView* mainView;
@end

@implementation APPRTCViewController {
  ARDAppClient* _client;
  RTCVideoTrack* _localVideoTrack;
  RTCVideoTrack* _remoteVideoTrack;
}

- (void)dealloc {
  [self disconnect];
}

- (void)loadView {
  APPRTCMainView* view = [[APPRTCMainView alloc] initWithFrame:NSZeroRect];
  [view setTranslatesAutoresizingMaskIntoConstraints:NO];
  view.delegate = self;
  self.view = view;
}

- (void)windowWillClose:(NSNotification*)notification {
  [self disconnect];
}

#pragma mark - ARDAppClientDelegate

- (void)appClient:(ARDAppClient *)client
    didChangeState:(ARDAppClientState)state {
  switch (state) {
    case kARDAppClientStateConnected:
      NSLog(@"Client connected.");
      break;
    case kARDAppClientStateConnecting:
      NSLog(@"Client connecting.");
      break;
    case kARDAppClientStateDisconnected:
      NSLog(@"Client disconnected.");
      [self resetUI];
      _client = nil;
      break;
  }
}

- (void)appClient:(ARDAppClient *)client
    didChangeConnectionState:(RTCIceConnectionState)state {
}

- (void)appClient:(ARDAppClient *)client
    didReceiveLocalVideoTrack:(RTCVideoTrack *)localVideoTrack {
  _localVideoTrack = localVideoTrack;
}

- (void)appClient:(ARDAppClient *)client
    didReceiveRemoteVideoTrack:(RTCVideoTrack *)remoteVideoTrack {
  _remoteVideoTrack = remoteVideoTrack;
  [_remoteVideoTrack addRenderer:self.mainView.remoteVideoView];
}

- (void)appClient:(ARDAppClient *)client
         didError:(NSError *)error {
  [self showAlertWithMessage:[NSString stringWithFormat:@"%@", error]];
  [self disconnect];
}

- (void)appClient:(ARDAppClient *)client
      didGetStats:(NSArray *)stats {
}

#pragma mark - APPRTCMainViewDelegate

- (void)appRTCMainView:(APPRTCMainView*)mainView
        didEnterRoomId:(NSString*)roomId {
  [_client disconnect];
  ARDAppClient *client = [[ARDAppClient alloc] initWithDelegate:self];
  [client connectToRoomWithId:roomId isLoopback:NO isAudioOnly:NO];
  _client = client;
}

#pragma mark - Private

- (APPRTCMainView*)mainView {
  return (APPRTCMainView*)self.view;
}

- (void)showAlertWithMessage:(NSString*)message {
  NSAlert* alert = [[NSAlert alloc] init];
  [alert setMessageText:message];
  [alert runModal];
}

- (void)resetUI {
  [_remoteVideoTrack removeRenderer:self.mainView.remoteVideoView];
  _remoteVideoTrack = nil;
  [self.mainView.remoteVideoView renderFrame:nil];
}

- (void)disconnect {
  [self resetUI];
  [_client disconnect];
}

@end
