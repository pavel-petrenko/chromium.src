// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/tray_view_controller.h"

#include <cmath>

#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/time.h"
#include "grit/ui_resources.h"
#include "grit/ui_strings.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/hover_image_button.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"
#import "ui/message_center/cocoa/notification_controller.h"
#import "ui/message_center/cocoa/settings_controller.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_style.h"

const int kBackButtonSize = 16;

// NSClipView subclass.
@interface MCClipView : NSClipView {
  // If this is set, the visible document area will remain intact no matter how
  // the user scrolls or drags the thumb.
  BOOL frozen_;
}
@end

@implementation MCClipView
- (void)setFrozen:(BOOL)frozen {
  frozen_ = frozen;
}

-(NSPoint)constrainScrollPoint:(NSPoint)proposedNewOrigin {
  return frozen_ ? [self documentVisibleRect].origin :
      [super constrainScrollPoint:proposedNewOrigin];
}
@end

@interface MCTrayViewController (Private)
// Creates all the views for the control area of the tray.
- (void)layoutControlArea;

// Update both tray view and window by resizing it to fit its content.
- (void)updateTrayViewAndWindow;

// Remove notifications dismissed by the user. It is done in the following
// 3 steps.
- (void)closeNotificationsByUser;

// Step 1: hide all notifications pending removal with fade-out animation.
- (void)hideNotificationsPendingRemoval;

// Step 2: move up all remaining notfications to take over the available space
// due to hiding notifications. The scroll view and the window remain unchanged.
- (void)moveUpRemainingNotifications;

// Step 3: finalize the tray view and window to get rid of the empty space.
- (void)finalizeTrayViewAndWindow;

// Clear a notification by sliding it out from left to right. This occurs when
// "Clear All" is clicked.
- (void)clearOneNotification;

// When all visible notificatons slide out, re-enable controls and remove
// notifications from the message center.
- (void)finalizeClearAll;
@end

namespace {

// The duration of fade-out and bounds animation.
const NSTimeInterval kAnimationDuration = 0.2;

// The delay to start animating clearing next notification since current
// animation starts.
const NSTimeInterval kAnimateClearingNextNotificationDelay = 0.04;

// The height of the bar at the top of the tray that contains buttons.
const CGFloat kControlAreaHeight = 50;

// Amount of spacing between control buttons. There is kMarginBetweenItems
// between a button and the edge of the tray, though.
const CGFloat kButtonXMargin = 20;

// Amount of padding to leave between the bottom of the screen and the bottom
// of the message center tray.
const CGFloat kTrayBottomMargin = 75;

}  // namespace

@implementation MCTrayViewController

- (id)initWithMessageCenter:(message_center::MessageCenter*)messageCenter {
  if ((self = [super initWithNibName:nil bundle:nil])) {
    messageCenter_ = messageCenter;
    animationDuration_ = kAnimationDuration;
    animateClearingNextNotificationDelay_ =
        kAnimateClearingNextNotificationDelay;
    notifications_.reset([[NSMutableArray alloc] init]);
    notificationsPendingRemoval_.reset([[NSMutableArray alloc] init]);
  }
  return self;
}

- (void)onWindowClosing {
  if (animation_) {
    [animation_ stopAnimation];
    [animation_ setDelegate:nil];
    animation_.reset();
  }
  if (clearAllInProgress_) {
    for (NSViewAnimation* animation in clearAllAnimations_.get()) {
      [animation stopAnimation];
      [animation setDelegate:nil];
    }
    [clearAllAnimations_ removeAllObjects];
    [self finalizeClearAll];
  }
}

- (void)loadView {
  // Configure the root view as a background-colored box.
  scoped_nsobject<NSBox> view([[NSBox alloc] initWithFrame:NSMakeRect(
      0, 0, [MCTrayViewController trayWidth], kControlAreaHeight)]);
  [view setBorderType:NSNoBorder];
  [view setBoxType:NSBoxCustom];
  [view setContentViewMargins:NSZeroSize];
  [view setFillColor:gfx::SkColorToCalibratedNSColor(
      message_center::kMessageCenterBackgroundColor)];
  [view setTitlePosition:NSNoTitle];
  [view setWantsLayer:YES];  // Needed for notification view shadows.
  [self setView:view];

  [self layoutControlArea];

  // Configure the scroll view in which all the notifications go.
  scoped_nsobject<NSView> documentView(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  scrollView_.reset([[NSScrollView alloc] initWithFrame:[view frame]]);
  clipView_.reset(
      [[MCClipView alloc] initWithFrame:[[scrollView_ contentView] frame]]);
  [scrollView_ setContentView:clipView_];
  [scrollView_ setAutohidesScrollers:YES];
  [scrollView_ setAutoresizingMask:NSViewHeightSizable | NSViewMaxYMargin];
  [scrollView_ setDocumentView:documentView];
  [scrollView_ setDrawsBackground:NO];
  [scrollView_ setHasHorizontalScroller:NO];
  [scrollView_ setHasVerticalScroller:YES];
  [view addSubview:scrollView_];

  [self onMessageCenterTrayChanged];
}

- (void)onMessageCenterTrayChanged {
  if (settingsController_)
    return [self updateTrayViewAndWindow];

  // When the window is visible, the only update is to remove notifications
  // dismissed by the user.
  if ([[[self view] window] isVisible]) {
    [self closeNotificationsByUser];
    return;
  }

  std::map<std::string, MCNotificationController*> newMap;

  scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
  [shadow setShadowColor:[NSColor colorWithDeviceWhite:0 alpha:0.55]];
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:2.0];

  CGFloat minY = message_center::kMarginBetweenItems;

  // Iterate over the notifications in reverse, since the Cocoa coordinate
  // origin is in the lower-left. Remove from |notificationsMap_| all the
  // ones still in the updated model, so that those that should be removed
  // will remain in the map.
  const auto& modelNotifications = messageCenter_->GetNotifications();
  for (auto it = modelNotifications.rbegin();
       it != modelNotifications.rend();
       ++it) {
    // Check if this notification is already in the tray.
    const auto& existing = notificationsMap_.find((*it)->id());
    MCNotificationController* notification = nil;
    if (existing == notificationsMap_.end()) {
      scoped_nsobject<MCNotificationController> controller(
          [[MCNotificationController alloc]
              initWithNotification:*it
                     messageCenter:messageCenter_]);
      [[controller view] setShadow:shadow];
      [[scrollView_ documentView] addSubview:[controller view]];

      [notifications_ addObject:controller];  // Transfer ownership.
      messageCenter_->DisplayedNotification((*it)->id());

      notification = controller.get();
    } else {
      notification = existing->second;
      [notification updateNotification:*it];
      notificationsMap_.erase(existing);
    }

    DCHECK(notification);

    NSRect frame = [[notification view] frame];
    frame.origin.x = message_center::kMarginBetweenItems;
    frame.origin.y = minY;
    [[notification view] setFrame:frame];

    newMap.insert(std::make_pair((*it)->id(), notification));

    minY = NSMaxY(frame) + message_center::kMarginBetweenItems;
  }

  // Remove any notifications that are no longer in the model.
  for (const auto& pair : notificationsMap_) {
    [[pair.second view] removeFromSuperview];
    [notifications_ removeObject:pair.second];
  }

  // Copy the new map of notifications to replace the old.
  notificationsMap_ = newMap;

  [self updateTrayViewAndWindow];
}

- (void)toggleQuietMode:(id)sender {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (messageCenter_->IsQuietMode()) {
    messageCenter_->SetQuietMode(false);
    [pauseButton_ setTrackingEnabled:YES];
    [pauseButton_ setDefaultImage:
        rb.GetNativeImageNamed(IDR_NOTIFICATION_PAUSE).ToNSImage()];
  } else {
    messageCenter_->EnterQuietModeWithExpire(base::TimeDelta::FromDays(1));
    [pauseButton_ setTrackingEnabled:NO];
    [pauseButton_ setDefaultImage:
        rb.GetNativeImageNamed(IDR_NOTIFICATION_PAUSE_PRESSED).ToNSImage()];
  }
}

- (void)clearAllNotifications:(id)sender {
  if ([self isAnimating]) {
    clearAllDelayed_ = YES;
    return;
  }

  // Build a list for all notifications within the visible scroll range
  // in preparation to slide them out one by one.
  NSRect visibleScrollRect = [scrollView_ documentVisibleRect];
  for (MCNotificationController* notification in notifications_.get()) {
    NSRect rect = [[notification view] frame];
    if (!NSIsEmptyRect(NSIntersectionRect(visibleScrollRect, rect))) {
      visibleNotificationsPendingClear_.push_back(notification);
    }
  }
  if (visibleNotificationsPendingClear_.empty())
    return;

  // Disbale buttons and freeze scroll bar to prevent the user from clicking on
  // them accidentally.
  [pauseButton_ setEnabled:NO];
  [clearAllButton_ setEnabled:NO];
  [settingsButton_ setEnabled:NO];
  [clipView_ setFrozen:YES];

  // Start sliding out the top notification.
  clearAllAnimations_.reset([[NSMutableArray alloc] init]);
  [self clearOneNotification];

  clearAllInProgress_ = YES;
}

- (void)showSettings:(id)sender {
  if (settingsController_)
    return [self hideSettings:sender];

  {
    // ShowNotificationSettingsDialog() returns an object owned by an
    // autoreleased controller. Use a local autorelease pool to make sure this
    // controller doesn't outlive self if self destroyed before the runloop
    // flushes autoreleased objects (for example, in tests).
    base::mac::ScopedNSAutoreleasePool pool;
    // This ends up calling message_center::ShowSettings() and will set up the
    // NotifierSettingsProvider along the way.
    message_center::NotifierSettingsDelegateMac* delegate =
        static_cast<message_center::NotifierSettingsDelegateMac*>(
            messageCenter_->ShowNotificationSettingsDialog([self view]));
    settingsController_.reset([delegate->cocoa_controller() retain]);
  }

  [[self view] addSubview:[settingsController_ view]];

  NSRect titleFrame = [title_ frame];
  titleFrame.origin.x =
      NSMaxX([backButton_ frame]) + message_center::kMarginBetweenItems / 2;
  [title_ setFrame:titleFrame];
  [backButton_ setHidden:NO];
  [clearAllButton_ setEnabled:NO];

  [[[self view] window] recalculateKeyViewLoop];

  [self updateTrayViewAndWindow];
}

- (void)hideSettings:(id)sender {
  [[settingsController_ view] removeFromSuperview];
  settingsController_.reset();

  NSRect titleFrame = [title_ frame];
  titleFrame.origin.x = NSMinX([backButton_ frame]);
  [title_ setFrame:titleFrame];
  [backButton_ setHidden:YES];
  [clearAllButton_ setEnabled:YES];

  [[[self view] window] recalculateKeyViewLoop];
  [self updateTrayViewAndWindow];
}

- (void)scrollToTop {
  NSPoint topPoint =
      NSMakePoint(0.0, [[scrollView_ documentView] bounds].size.height);
  [[scrollView_ documentView] scrollPoint:topPoint];
}

- (BOOL)isAnimating {
  return [animation_ isAnimating] || [clearAllAnimations_ count];
}

+ (CGFloat)maxTrayClientHeight {
  NSRect screenFrame = [[[NSScreen screens] objectAtIndex:0] visibleFrame];
  return NSHeight(screenFrame) - kTrayBottomMargin - kControlAreaHeight;
}

+ (CGFloat)trayWidth {
  return message_center::kNotificationWidth +
         2 * message_center::kMarginBetweenItems;
}

// Testing API /////////////////////////////////////////////////////////////////

- (NSScrollView*)scrollView {
  return scrollView_.get();
}

- (HoverImageButton*)pauseButton {
  return pauseButton_.get();
}

- (HoverImageButton*)clearAllButton {
  return clearAllButton_.get();
}

- (void)setAnimationDuration:(NSTimeInterval)duration {
  animationDuration_ = duration;
}

- (void)setAnimateClearingNextNotificationDelay:(NSTimeInterval)delay {
  animateClearingNextNotificationDelay_ = delay;
}

- (void)setAnimationEndedCallback:
    (message_center::TrayAnimationEndedCallback)callback {
  testingAnimationEndedCallback_.reset(Block_copy(callback));
}

// Private /////////////////////////////////////////////////////////////////////

- (void)layoutControlArea {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSView* view = [self view];

  // Create the "Notifications" label at the top of the tray.
  NSFont* font = [NSFont labelFontOfSize:message_center::kTitleFontSize];
  title_.reset([[NSTextField alloc] initWithFrame:NSZeroRect]);
  [title_ setAutoresizingMask:NSViewMinYMargin];
  [title_ setBezeled:NO];
  [title_ setBordered:NO];
  [title_ setDrawsBackground:NO];
  [title_ setEditable:NO];
  [title_ setFont:font];
  [title_ setSelectable:NO];
  [title_ setStringValue:
      l10n_util::GetNSString(IDS_MESSAGE_CENTER_FOOTER_TITLE)];
  [title_ setTextColor:gfx::SkColorToCalibratedNSColor(
      message_center::kRegularTextColor)];
  [title_ sizeToFit];

  NSRect titleFrame = [title_ frame];
  titleFrame.origin.x = message_center::kMarginBetweenItems;
  titleFrame.origin.y = kControlAreaHeight/2 - NSMidY(titleFrame);
  [title_ setFrame:titleFrame];
  [view addSubview:title_];

  auto configureButton = ^(HoverImageButton* button) {
      [[button cell] setHighlightsBy:NSOnState];
      [button setTrackingEnabled:YES];
      [button setBordered:NO];
      [button setAutoresizingMask:NSViewMinYMargin];
      [button setTarget:self];
  };

  // Back button. On top of the "Notifications" label, hidden by default.
  NSRect backButtonFrame =
      NSMakeRect(NSMinX(titleFrame),
                 (kControlAreaHeight - kBackButtonSize) / 2,
                 kBackButtonSize,
                 kBackButtonSize);
  backButton_.reset([[HoverImageButton alloc] initWithFrame:backButtonFrame]);
  [backButton_ setDefaultImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_ARROW).ToNSImage()];
  [backButton_ setHoverImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_ARROW_HOVER).ToNSImage()];
  [backButton_ setPressedImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_ARROW_PRESSED).ToNSImage()];
  [backButton_ setAction:@selector(hideSettings:)];
  configureButton(backButton_);
  [backButton_ setHidden:YES];
  [backButton_ setKeyEquivalent:@"\e"];
  [[self view] addSubview:backButton_];

  // Create the divider line between the control area and the notifications.
  scoped_nsobject<NSBox> divider(
      [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, NSWidth([view frame]), 1)]);
  [divider setAutoresizingMask:NSViewMinYMargin];
  [divider setBorderType:NSNoBorder];
  [divider setBoxType:NSBoxCustom];
  [divider setContentViewMargins:NSZeroSize];
  [divider setFillColor:gfx::SkColorToCalibratedNSColor(
      message_center::kFooterDelimiterColor)];
  [divider setTitlePosition:NSNoTitle];
  [view addSubview:divider];

  auto getButtonFrame = ^NSRect(CGFloat maxX, NSImage* image) {
      NSSize size = [image size];
      return NSMakeRect(
          maxX - size.width,
          kControlAreaHeight/2 - size.height/2,
          size.width,
          size.height);
  };

  // Create the settings button at the far-right.
  NSImage* defaultImage =
      rb.GetNativeImageNamed(IDR_NOTIFICATION_SETTINGS).ToNSImage();
  NSRect settingsButtonFrame = getButtonFrame(
      NSWidth([view frame]) - message_center::kMarginBetweenItems,
      defaultImage);
  settingsButton_.reset(
      [[HoverImageButton alloc] initWithFrame:settingsButtonFrame]);
  [settingsButton_ setDefaultImage:defaultImage];
  [settingsButton_ setHoverImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_SETTINGS_HOVER).ToNSImage()];
  [settingsButton_ setPressedImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_SETTINGS_PRESSED).ToNSImage()];
  [settingsButton_ setToolTip:
      l10n_util::GetNSString(IDS_MESSAGE_CENTER_SETTINGS_BUTTON_LABEL)];
  [settingsButton_ setAction:@selector(showSettings:)];
  configureButton(settingsButton_);
  [view addSubview:settingsButton_];

  // Create the clear all button.
  defaultImage = rb.GetNativeImageNamed(IDR_NOTIFICATION_CLEAR_ALL).ToNSImage();
  NSRect clearAllButtonFrame = getButtonFrame(
      NSMinX(settingsButtonFrame) - kButtonXMargin,
      defaultImage);
  clearAllButton_.reset(
      [[HoverImageButton alloc] initWithFrame:clearAllButtonFrame]);
  [clearAllButton_ setDefaultImage:defaultImage];
  [clearAllButton_ setHoverImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_CLEAR_ALL_HOVER).ToNSImage()];
  [clearAllButton_ setPressedImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_CLEAR_ALL_PRESSED).ToNSImage()];
  [clearAllButton_ setToolTip:
      l10n_util::GetNSString(IDS_MESSAGE_CENTER_CLEAR_ALL)];
  [clearAllButton_ setAction:@selector(clearAllNotifications:)];
  configureButton(clearAllButton_);
  [view addSubview:clearAllButton_];

  // Create the pause button.
  defaultImage = rb.GetNativeImageNamed(IDR_NOTIFICATION_PAUSE).ToNSImage();
  NSRect pauseButtonFrame = getButtonFrame(
      NSMinX(clearAllButtonFrame) - kButtonXMargin,
      defaultImage);
  pauseButton_.reset([[HoverImageButton alloc] initWithFrame:pauseButtonFrame]);
  [pauseButton_ setDefaultImage:defaultImage];
  [pauseButton_ setHoverImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_PAUSE_HOVER).ToNSImage()];
  [pauseButton_ setPressedImage:
      rb.GetNativeImageNamed(IDR_NOTIFICATION_PAUSE_PRESSED).ToNSImage()];
  [pauseButton_ setToolTip:
      l10n_util::GetNSString(IDS_MESSAGE_CENTER_QUIET_MODE_BUTTON_TOOLTIP)];
  [pauseButton_ setAction:@selector(toggleQuietMode:)];
  configureButton(pauseButton_);
  [view addSubview:pauseButton_];
}

- (void)updateTrayViewAndWindow {
  CGFloat scrollContentHeight = 0;
  if ([notifications_ count]) {
    scrollContentHeight = NSMaxY([[[notifications_ lastObject] view] frame]) +
        message_center::kMarginBetweenItems;;
  }

  // Resize the scroll view's content.
  NSRect scrollViewFrame = [scrollView_ frame];
  NSRect documentFrame = [[scrollView_ documentView] frame];
  documentFrame.size.width = NSWidth(scrollViewFrame);
  documentFrame.size.height = scrollContentHeight;
  [[scrollView_ documentView] setFrame:documentFrame];

  // Resize the container view.
  NSRect frame = [[self view] frame];
  CGFloat oldHeight = NSHeight(frame);
  if (settingsController_) {
    frame.size.height = NSHeight([[settingsController_ view] frame]);
  } else {
    frame.size.height = std::min([MCTrayViewController maxTrayClientHeight],
                                 scrollContentHeight);
  }
  frame.size.height += kControlAreaHeight;
  CGFloat newHeight = NSHeight(frame);
  [[self view] setFrame:frame];

  // Resize the scroll view.
  scrollViewFrame.size.height = NSHeight(frame) - kControlAreaHeight;
  [scrollView_ setFrame:scrollViewFrame];

  // Resize the window.
  NSRect windowFrame = [[[self view] window] frame];
  CGFloat delta = newHeight - oldHeight;
  windowFrame.origin.y -= delta;
  windowFrame.size.height += delta;

  [[[self view] window] setFrame:windowFrame display:YES];
  // Hide the clear-all button if there are no notifications. Simply swap the
  // X position of it and the pause button in that case.
  BOOL hidden = ![notifications_ count];
  if ([clearAllButton_ isHidden] != hidden) {
    [clearAllButton_ setHidden:hidden];

    NSRect pauseButtonFrame = [pauseButton_ frame];
    NSRect clearAllButtonFrame = [clearAllButton_ frame];
    std::swap(clearAllButtonFrame.origin.x, pauseButtonFrame.origin.x);
    [pauseButton_ setFrame:pauseButtonFrame];
    [clearAllButton_ setFrame:clearAllButtonFrame];
  }
}

- (void)animationDidEnd:(NSAnimation*)animation {
  if (clearAllInProgress_) {
    // For clear-all animation.
    [clearAllAnimations_ removeObject:animation];
    if (![clearAllAnimations_ count] &&
        visibleNotificationsPendingClear_.empty()) {
      [self finalizeClearAll];
    }
  } else {
    // For notification removal and reposition animation.
    if ([notificationsPendingRemoval_ count]) {
      [self moveUpRemainingNotifications];
    } else {
      [self finalizeTrayViewAndWindow];

      if (clearAllDelayed_)
        [self clearAllNotifications:nil];
    }
  }

  // Give the testing code a chance to do something, i.e. quitting the test
  // run loop.
  if (![self isAnimating] && testingAnimationEndedCallback_)
    testingAnimationEndedCallback_.get()();
}

- (void)closeNotificationsByUser {
  // No need to close individual notification if clear-all is in progress.
  if (clearAllInProgress_)
    return;

  if ([self isAnimating])
    return;
  [self hideNotificationsPendingRemoval];
}

- (void)hideNotificationsPendingRemoval {
  scoped_nsobject<NSMutableArray> animationDataArray(
      [[NSMutableArray alloc] init]);

  // Fade-out those notifications pending removal.
  for (MCNotificationController* notification in notifications_.get()) {
    if (messageCenter_->HasNotification([notification notificationID]))
      continue;
    [notificationsPendingRemoval_ addObject:notification];
    [animationDataArray addObject:@{
        NSViewAnimationTargetKey : [notification view],
        NSViewAnimationEffectKey : NSViewAnimationFadeOutEffect
    }];
  }

  if ([notificationsPendingRemoval_ count] == 0)
    return;

  for (MCNotificationController* notification in
           notificationsPendingRemoval_.get()) {
    [notifications_ removeObject:notification];
  }

  // Start the animation.
  animation_.reset([[NSViewAnimation alloc]
      initWithViewAnimations:animationDataArray]);
  [animation_ setDuration:animationDuration_];
  [animation_ setDelegate:self];
  [animation_ startAnimation];
}

- (void)moveUpRemainingNotifications {
  scoped_nsobject<NSMutableArray> animationDataArray(
      [[NSMutableArray alloc] init]);

  // Compute the position where the remaining notifications should start.
  CGFloat minY = message_center::kMarginBetweenItems;
  for (MCNotificationController* notification in
           notificationsPendingRemoval_.get()) {
    NSView* view = [notification view];
    minY += NSHeight([view frame]) + message_center::kMarginBetweenItems;
  }

  // Reposition the remaining notifications starting at the computed position.
  for (MCNotificationController* notification in notifications_.get()) {
    NSView* view = [notification view];
    NSRect frame = [view frame];
    NSRect oldFrame = frame;
    frame.origin.y = minY;
    if (!NSEqualRects(oldFrame, frame)) {
      [animationDataArray addObject:@{
          NSViewAnimationTargetKey : view,
          NSViewAnimationEndFrameKey : [NSValue valueWithRect:frame]
      }];
    }
    minY = NSMaxY(frame) + message_center::kMarginBetweenItems;
  }

  // Now remove notifications pending removal.
  for (MCNotificationController* notification in
           notificationsPendingRemoval_.get()) {
    [[notification view] removeFromSuperview];
    notificationsMap_.erase([notification notificationID]);
  }
  [notificationsPendingRemoval_ removeAllObjects];

  // Start the animation.
  animation_.reset([[NSViewAnimation alloc]
      initWithViewAnimations:animationDataArray]);
  [animation_ setDuration:animationDuration_];
  [animation_ setDelegate:self];
  [animation_ startAnimation];
}

- (void)finalizeTrayViewAndWindow {
  // Reposition the remaining notifications starting at the bottom.
  CGFloat minY = message_center::kMarginBetweenItems;
  for (MCNotificationController* notification in notifications_.get()) {
    NSView* view = [notification view];
    NSRect frame = [view frame];
    NSRect oldFrame = frame;
    frame.origin.y = minY;
    if (!NSEqualRects(oldFrame, frame))
      [view setFrame:frame];
    minY = NSMaxY(frame) + message_center::kMarginBetweenItems;
  }

  [self updateTrayViewAndWindow];

  // Check if there're more notifications pending removal.
  [self closeNotificationsByUser];
}

- (void)clearOneNotification {
  DCHECK(!visibleNotificationsPendingClear_.empty());

  MCNotificationController* notification =
      visibleNotificationsPendingClear_.back();
  visibleNotificationsPendingClear_.pop_back();

  // Slide out the notification from left to right with fade-out simultaneously.
  NSRect newFrame = [[notification view] frame];
  newFrame.origin.x = NSMaxX(newFrame) + message_center::kMarginBetweenItems;
  NSDictionary* animationDict = @{
    NSViewAnimationTargetKey : [notification view],
    NSViewAnimationEndFrameKey : [NSValue valueWithRect:newFrame],
    NSViewAnimationEffectKey : NSViewAnimationFadeOutEffect
  };
  scoped_nsobject<NSViewAnimation> animation([[NSViewAnimation alloc]
      initWithViewAnimations:[NSArray arrayWithObject:animationDict]]);
  [animation setDuration:animationDuration_];
  [animation setDelegate:self];
  [animation startAnimation];
  [clearAllAnimations_ addObject:animation];

  // Schedule to start sliding out next notification after a short delay.
  if (!visibleNotificationsPendingClear_.empty()) {
    [self performSelector:@selector(clearOneNotification)
               withObject:nil
               afterDelay:animateClearingNextNotificationDelay_];
  }
}

- (void)finalizeClearAll {
  DCHECK(clearAllInProgress_);
  clearAllInProgress_ = NO;

  DCHECK(![clearAllAnimations_ count]);
  clearAllAnimations_.reset();

  [pauseButton_ setEnabled:YES];
  [clearAllButton_ setEnabled:YES];
  [settingsButton_ setEnabled:YES];
  [clipView_ setFrozen:NO];

  messageCenter_->RemoveAllNotifications(true);
}

@end
