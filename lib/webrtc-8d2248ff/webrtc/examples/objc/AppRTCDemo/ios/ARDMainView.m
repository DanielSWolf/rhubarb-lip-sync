/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "ARDMainView.h"

#import "UIImage+ARDUtilities.h"

// TODO(tkchin): retrieve status bar height dynamically.
static CGFloat const kStatusBarHeight = 20;

static CGFloat const kRoomTextButtonSize = 40;
static CGFloat const kRoomTextFieldHeight = 40;
static CGFloat const kRoomTextFieldMargin = 8;
static CGFloat const kCallControlMargin = 8;
static CGFloat const kAppLabelHeight = 20;

// Helper view that contains a text field and a clear button.
@interface ARDRoomTextField : UIView <UITextFieldDelegate>
@property(nonatomic, readonly) NSString *roomText;
@end

@implementation ARDRoomTextField {
  UITextField *_roomText;
  UIButton *_clearButton;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    _roomText = [[UITextField alloc] initWithFrame:CGRectZero];
    _roomText.borderStyle = UITextBorderStyleNone;
    _roomText.font = [UIFont fontWithName:@"Roboto" size:12];
    _roomText.placeholder = @"Room name";
    _roomText.autocorrectionType = UITextAutocorrectionTypeNo;
    _roomText.autocapitalizationType = UITextAutocapitalizationTypeNone;
    _roomText.delegate = self;
    [_roomText addTarget:self
                  action:@selector(textFieldDidChange:)
        forControlEvents:UIControlEventEditingChanged];
    [self addSubview:_roomText];

    _clearButton = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage *image = [UIImage imageForName:@"ic_clear_black_24dp.png"
                                     color:[UIColor colorWithWhite:0 alpha:.4]];

    [_clearButton setImage:image forState:UIControlStateNormal];
    [_clearButton addTarget:self
                      action:@selector(onClear:)
            forControlEvents:UIControlEventTouchUpInside];
    _clearButton.hidden = YES;
    [self addSubview:_clearButton];

    // Give rounded corners and a light gray border.
    self.layer.borderWidth = 1;
    self.layer.borderColor = [[UIColor lightGrayColor] CGColor];
    self.layer.cornerRadius = 2;
  }
  return self;
}

- (void)layoutSubviews {
  CGRect bounds = self.bounds;
  _clearButton.frame = CGRectMake(CGRectGetMaxX(bounds) - kRoomTextButtonSize,
                                  CGRectGetMinY(bounds),
                                  kRoomTextButtonSize,
                                  kRoomTextButtonSize);
  _roomText.frame = CGRectMake(
      CGRectGetMinX(bounds) + kRoomTextFieldMargin,
      CGRectGetMinY(bounds),
      CGRectGetMinX(_clearButton.frame) - CGRectGetMinX(bounds) -
          kRoomTextFieldMargin,
      kRoomTextFieldHeight);
}

- (CGSize)sizeThatFits:(CGSize)size {
  size.height = kRoomTextFieldHeight;
  return size;
}

- (NSString *)roomText {
  return _roomText.text;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
  // There is no other control that can take focus, so manually resign focus
  // when return (Join) is pressed to trigger |textFieldDidEndEditing|.
  [textField resignFirstResponder];
  return YES;
}

#pragma mark - Private

- (void)textFieldDidChange:(id)sender {
  [self updateClearButton];
}

- (void)onClear:(id)sender {
  _roomText.text = @"";
  [self updateClearButton];
  [_roomText resignFirstResponder];
}

- (void)updateClearButton {
  _clearButton.hidden = _roomText.text.length == 0;
}

@end

@implementation ARDMainView {
  UILabel *_appLabel;
  ARDRoomTextField *_roomText;
  UILabel *_callOptionsLabel;
  UISwitch *_audioOnlySwitch;
  UILabel *_audioOnlyLabel;
  UISwitch *_loopbackSwitch;
  UILabel *_loopbackLabel;
  UISwitch *_useManualAudioSwitch;
  UILabel *_useManualAudioLabel;
  UIButton *_startCallButton;
  UIButton *_audioLoopButton;
}

@synthesize delegate = _delegate;
@synthesize isAudioLoopPlaying = _isAudioLoopPlaying;

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    _appLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _appLabel.text = @"AppRTCDemo";
    _appLabel.font = [UIFont fontWithName:@"Roboto" size:34];
    _appLabel.textColor = [UIColor colorWithWhite:0 alpha:.2];
    [_appLabel sizeToFit];
    [self addSubview:_appLabel];

    _roomText = [[ARDRoomTextField alloc] initWithFrame:CGRectZero];
    [self addSubview:_roomText];

    UIFont *controlFont = [UIFont fontWithName:@"Roboto" size:20];
    UIColor *controlFontColor = [UIColor colorWithWhite:0 alpha:.6];

    _callOptionsLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _callOptionsLabel.text = @"Call Options";
    _callOptionsLabel.font = controlFont;
    _callOptionsLabel.textColor = controlFontColor;
    [_callOptionsLabel sizeToFit];
    [self addSubview:_callOptionsLabel];

    _audioOnlySwitch = [[UISwitch alloc] initWithFrame:CGRectZero];
    [_audioOnlySwitch sizeToFit];
    [self addSubview:_audioOnlySwitch];

    _audioOnlyLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _audioOnlyLabel.text = @"Audio only";
    _audioOnlyLabel.font = controlFont;
    _audioOnlyLabel.textColor = controlFontColor;
    [_audioOnlyLabel sizeToFit];
    [self addSubview:_audioOnlyLabel];

    _loopbackSwitch = [[UISwitch alloc] initWithFrame:CGRectZero];
    [_loopbackSwitch sizeToFit];
    [self addSubview:_loopbackSwitch];

    _loopbackLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _loopbackLabel.text = @"Loopback mode";
    _loopbackLabel.font = controlFont;
    _loopbackLabel.textColor = controlFontColor;
    [_loopbackLabel sizeToFit];
    [self addSubview:_loopbackLabel];

    _useManualAudioSwitch = [[UISwitch alloc] initWithFrame:CGRectZero];
    [_useManualAudioSwitch sizeToFit];
    _useManualAudioSwitch.on = YES;
    [self addSubview:_useManualAudioSwitch];

    _useManualAudioLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _useManualAudioLabel.text = @"Use manual audio config";
    _useManualAudioLabel.font = controlFont;
    _useManualAudioLabel.textColor = controlFontColor;
    [_useManualAudioLabel sizeToFit];
    [self addSubview:_useManualAudioLabel];

    _startCallButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _startCallButton.backgroundColor = [UIColor blueColor];
    _startCallButton.layer.cornerRadius = 10;
    _startCallButton.clipsToBounds = YES;
    _startCallButton.contentEdgeInsets = UIEdgeInsetsMake(5, 10, 5, 10);
    [_startCallButton setTitle:@"Start call"
                      forState:UIControlStateNormal];
    _startCallButton.titleLabel.font = controlFont;
    [_startCallButton setTitleColor:[UIColor whiteColor]
                           forState:UIControlStateNormal];
    [_startCallButton setTitleColor:[UIColor lightGrayColor]
                           forState:UIControlStateSelected];
    [_startCallButton sizeToFit];
    [_startCallButton addTarget:self
                         action:@selector(onStartCall:)
               forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:_startCallButton];

    // Used to test what happens to sounds when calls are in progress.
    _audioLoopButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _audioLoopButton.layer.cornerRadius = 10;
    _audioLoopButton.clipsToBounds = YES;
    _audioLoopButton.contentEdgeInsets = UIEdgeInsetsMake(5, 10, 5, 10);
    _audioLoopButton.titleLabel.font = controlFont;
    [_audioLoopButton setTitleColor:[UIColor whiteColor]
                           forState:UIControlStateNormal];
    [_audioLoopButton setTitleColor:[UIColor lightGrayColor]
                           forState:UIControlStateSelected];
    [self updateAudioLoopButton];
    [_audioLoopButton addTarget:self
                         action:@selector(onToggleAudioLoop:)
               forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:_audioLoopButton];

    self.backgroundColor = [UIColor whiteColor];
  }
  return self;
}

- (void)setIsAudioLoopPlaying:(BOOL)isAudioLoopPlaying {
  if (_isAudioLoopPlaying == isAudioLoopPlaying) {
    return;
  }
  _isAudioLoopPlaying = isAudioLoopPlaying;
  [self updateAudioLoopButton];
}

- (void)layoutSubviews {
  CGRect bounds = self.bounds;
  CGFloat roomTextWidth = bounds.size.width - 2 * kRoomTextFieldMargin;
  CGFloat roomTextHeight = [_roomText sizeThatFits:bounds.size].height;
  _roomText.frame = CGRectMake(kRoomTextFieldMargin,
                               kStatusBarHeight + kRoomTextFieldMargin,
                               roomTextWidth,
                               roomTextHeight);
  _appLabel.center = CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));

  CGFloat callOptionsLabelTop =
      CGRectGetMaxY(_roomText.frame) + kCallControlMargin * 4;
  _callOptionsLabel.frame = CGRectMake(kCallControlMargin,
                                       callOptionsLabelTop,
                                       _callOptionsLabel.frame.size.width,
                                       _callOptionsLabel.frame.size.height);

  CGFloat audioOnlyTop =
      CGRectGetMaxY(_callOptionsLabel.frame) + kCallControlMargin * 2;
  CGRect audioOnlyRect = CGRectMake(kCallControlMargin * 3,
                                    audioOnlyTop,
                                    _audioOnlySwitch.frame.size.width,
                                    _audioOnlySwitch.frame.size.height);
  _audioOnlySwitch.frame = audioOnlyRect;
  CGFloat audioOnlyLabelCenterX = CGRectGetMaxX(audioOnlyRect) +
      kCallControlMargin + _audioOnlyLabel.frame.size.width / 2;
  _audioOnlyLabel.center = CGPointMake(audioOnlyLabelCenterX,
                                       CGRectGetMidY(audioOnlyRect));

  CGFloat loopbackModeTop =
      CGRectGetMaxY(_audioOnlySwitch.frame) + kCallControlMargin;
  CGRect loopbackModeRect = CGRectMake(kCallControlMargin * 3,
                                       loopbackModeTop,
                                       _loopbackSwitch.frame.size.width,
                                       _loopbackSwitch.frame.size.height);
  _loopbackSwitch.frame = loopbackModeRect;
  CGFloat loopbackModeLabelCenterX = CGRectGetMaxX(loopbackModeRect) +
      kCallControlMargin + _loopbackLabel.frame.size.width / 2;
  _loopbackLabel.center = CGPointMake(loopbackModeLabelCenterX,
                                      CGRectGetMidY(loopbackModeRect));

  CGFloat useManualAudioTop =
      CGRectGetMaxY(_loopbackSwitch.frame) + kCallControlMargin;
  CGRect useManualAudioRect =
      CGRectMake(kCallControlMargin * 3,
                 useManualAudioTop,
                 _useManualAudioSwitch.frame.size.width,
                 _useManualAudioSwitch.frame.size.height);
  _useManualAudioSwitch.frame = useManualAudioRect;
  CGFloat useManualAudioLabelCenterX = CGRectGetMaxX(useManualAudioRect) +
      kCallControlMargin + _useManualAudioLabel.frame.size.width / 2;
  _useManualAudioLabel.center =
      CGPointMake(useManualAudioLabelCenterX,
                  CGRectGetMidY(useManualAudioRect));

  CGFloat audioLoopTop =
     CGRectGetMaxY(useManualAudioRect) + kCallControlMargin * 3;
  _audioLoopButton.frame = CGRectMake(kCallControlMargin,
                                      audioLoopTop,
                                      _audioLoopButton.frame.size.width,
                                      _audioLoopButton.frame.size.height);

  CGFloat startCallTop =
      CGRectGetMaxY(_audioLoopButton.frame) + kCallControlMargin * 3;
  _startCallButton.frame = CGRectMake(kCallControlMargin,
                                      startCallTop,
                                      _startCallButton.frame.size.width,
                                      _startCallButton.frame.size.height);
}

#pragma mark - Private

- (void)updateAudioLoopButton {
  if (_isAudioLoopPlaying) {
    _audioLoopButton.backgroundColor = [UIColor redColor];
    [_audioLoopButton setTitle:@"Stop sound"
                      forState:UIControlStateNormal];
    [_audioLoopButton sizeToFit];
  } else {
    _audioLoopButton.backgroundColor = [UIColor greenColor];
    [_audioLoopButton setTitle:@"Play sound"
                      forState:UIControlStateNormal];
    [_audioLoopButton sizeToFit];
  }
}

- (void)onToggleAudioLoop:(id)sender {
  [_delegate mainViewDidToggleAudioLoop:self];
}

- (void)onStartCall:(id)sender {
  NSString *room = _roomText.roomText;
  // If this is a loopback call, allow a generated room name.
  if (!room.length && _loopbackSwitch.isOn) {
    room = [[NSUUID UUID] UUIDString];
  }
  room = [room stringByReplacingOccurrencesOfString:@"-" withString:@""];
  [_delegate mainView:self
                didInputRoom:room
                  isLoopback:_loopbackSwitch.isOn
                 isAudioOnly:_audioOnlySwitch.isOn
              useManualAudio:_useManualAudioSwitch.isOn];
}

@end
