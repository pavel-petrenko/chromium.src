// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "remoting/host/disconnect_window_mac.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "remoting/host/client_session_control.h"
#include "remoting/host/host_window.h"
#include "remoting/host/ui_strings.h"

@interface DisconnectWindowController()
- (BOOL)isRToL;
- (void)Hide;
@end

const int kMaximumConnectedNameWidthInPixels = 400;

namespace remoting {

class DisconnectWindowMac : public HostWindow {
 public:
  explicit DisconnectWindowMac(const UiStrings& ui_strings);
  virtual ~DisconnectWindowMac();

  // HostWindow overrides.
  virtual void Start(
      const base::WeakPtr<ClientSessionControl>& client_session_control)
      OVERRIDE;

 private:
  // Localized UI strings.
  UiStrings ui_strings_;

  DisconnectWindowController* window_controller_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectWindowMac);
};

DisconnectWindowMac::DisconnectWindowMac(const UiStrings& ui_strings)
    : ui_strings_(ui_strings),
      window_controller_(nil) {
}

DisconnectWindowMac::~DisconnectWindowMac() {
  DCHECK(CalledOnValidThread());

  // DisconnectWindowController is responsible for releasing itself in its
  // windowWillClose: method.
  [window_controller_ Hide];
  window_controller_ = nil;
}

void DisconnectWindowMac::Start(
    const base::WeakPtr<ClientSessionControl>& client_session_control) {
  DCHECK(CalledOnValidThread());
  DCHECK(client_session_control);
  DCHECK(window_controller_ == nil);

  // Create the window.
  base::Closure disconnect_callback =
      base::Bind(&ClientSessionControl::DisconnectSession,
                 client_session_control);
  std::string client_jid = client_session_control->client_jid();
  std::string username = client_jid.substr(0, client_jid.find('/'));
  window_controller_ =
      [[DisconnectWindowController alloc] initWithUiStrings:&ui_strings_
                                                   callback:disconnect_callback
                                                   username:username];
  [window_controller_ showWindow:nil];
}

// static
scoped_ptr<HostWindow> HostWindow::CreateDisconnectWindow(
    const UiStrings& ui_strings) {
  return scoped_ptr<HostWindow>(new DisconnectWindowMac(ui_strings));
}

}  // namespace remoting

@implementation DisconnectWindowController
- (id)initWithUiStrings:(const remoting::UiStrings*)ui_strings
               callback:(const base::Closure&)disconnect_callback
               username:(const std::string&)username {
  self = [super initWithWindowNibName:@"disconnect_window"];
  if (self) {
    ui_strings_ = ui_strings;
    disconnect_callback_ = disconnect_callback;
    username_ = UTF8ToUTF16(username);
  }
  return self;
}

- (void)dealloc {
  [super dealloc];
}

- (IBAction)stopSharing:(id)sender {
  if (!disconnect_callback_.is_null()) {
    disconnect_callback_.Run();
  }
}

- (BOOL)isRToL {
  return ui_strings_->direction == remoting::UiStrings::RTL;
}

- (void)Hide {
  disconnect_callback_.Reset();
  [self close];
}

- (void)windowDidLoad {
  string16 text = ReplaceStringPlaceholders(ui_strings_->disconnect_message,
                                            username_, NULL);
  [connectedToField_ setStringValue:base::SysUTF16ToNSString(text)];

  [disconnectButton_ setTitle:base::SysUTF16ToNSString(
      ui_strings_->disconnect_button_text)];

  // Resize the window dynamically based on the content.
  CGFloat oldConnectedWidth = NSWidth([connectedToField_ bounds]);
  [connectedToField_ sizeToFit];
  NSRect connectedToFrame = [connectedToField_ frame];
  CGFloat newConnectedWidth = NSWidth(connectedToFrame);

  // Set a max width for the connected to text field.
  if (newConnectedWidth > kMaximumConnectedNameWidthInPixels) {
    newConnectedWidth = kMaximumConnectedNameWidthInPixels;
    connectedToFrame.size.width = newConnectedWidth;
    [connectedToField_ setFrame:connectedToFrame];
  }

  CGFloat oldDisconnectWidth = NSWidth([disconnectButton_ bounds]);
  [disconnectButton_ sizeToFit];
  NSRect disconnectFrame = [disconnectButton_ frame];
  CGFloat newDisconnectWidth = NSWidth(disconnectFrame);

  // Move the disconnect button appropriately.
  disconnectFrame.origin.x += newConnectedWidth - oldConnectedWidth;
  [disconnectButton_ setFrame:disconnectFrame];

  // Then resize the window appropriately
  NSWindow *window = [self window];
  NSRect windowFrame = [window frame];
  windowFrame.size.width += (newConnectedWidth - oldConnectedWidth +
                             newDisconnectWidth - oldDisconnectWidth);
  [window setFrame:windowFrame display:NO];

  if ([self isRToL]) {
    // Handle right to left case
    CGFloat buttonInset = NSWidth(windowFrame) - NSMaxX(disconnectFrame);
    CGFloat buttonTextSpacing
        = NSMinX(disconnectFrame) - NSMaxX(connectedToFrame);
    disconnectFrame.origin.x = buttonInset;
    connectedToFrame.origin.x = NSMaxX(disconnectFrame) + buttonTextSpacing;
    [connectedToField_ setFrame:connectedToFrame];
    [disconnectButton_ setFrame:disconnectFrame];
  }

  // Center the window at the bottom of the screen, above the dock (if present).
  NSRect desktopRect = [[NSScreen mainScreen] visibleFrame];
  NSRect windowRect = [[self window] frame];
  CGFloat x = (NSWidth(desktopRect) - NSWidth(windowRect)) / 2;
  CGFloat y = NSMinY(desktopRect);
  [[self window] setFrameOrigin:NSMakePoint(x, y)];
}

- (void)windowWillClose:(NSNotification*)notification {
  [self stopSharing:self];
  [self autorelease];
}

@end


@interface DisconnectWindow()
- (BOOL)isRToL;
@end

@implementation DisconnectWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)aStyle
                  backing:(NSBackingStoreType)bufferingType
                  defer:(BOOL)flag {
  // Pass NSBorderlessWindowMask for the styleMask to remove the title bar.
  self = [super initWithContentRect:contentRect
                          styleMask:NSBorderlessWindowMask
                            backing:bufferingType
                              defer:flag];

  if (self) {
    // Set window to be clear and non-opaque so we can see through it.
    [self setBackgroundColor:[NSColor clearColor]];
    [self setOpaque:NO];
    [self setMovableByWindowBackground:YES];

    // Pull the window up to Status Level so that it always displays.
    [self setLevel:NSStatusWindowLevel];
  }
  return self;
}

- (BOOL)isRToL {
  DCHECK([[self windowController] respondsToSelector:@selector(isRToL)]);
  return [[self windowController] isRToL];
}

@end


@interface DisconnectView()
- (BOOL)isRToL;
@end

@implementation DisconnectView

- (BOOL)isRToL {
  DCHECK([[self window] isKindOfClass:[DisconnectWindow class]]);
  return [static_cast<DisconnectWindow*>([self window]) isRToL];
}

- (void)drawRect:(NSRect)rect {
  // All magic numbers taken from screen shots provided by UX.
  NSRect bounds = NSInsetRect([self bounds], 1, 1);

  NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:bounds
                                                       xRadius:5
                                                       yRadius:5];
  NSColor *gray = [NSColor colorWithCalibratedWhite:0.91 alpha:1.0];
  [gray setFill];
  [path fill];
  [path setLineWidth:4];
  NSColor *green = [NSColor colorWithCalibratedRed:0.13
                                             green:0.69
                                              blue:0.11
                                             alpha:1.0];
  [green setStroke];
  [path stroke];


  // Draw drag handle on proper side
  const CGFloat kHeight = 21.0;
  const CGFloat kBaseInset = 12.0;
  const CGFloat kDragHandleWidth = 5.0;

  NSColor *dark = [NSColor colorWithCalibratedWhite:0.70 alpha:1.0];
  NSColor *light = [NSColor colorWithCalibratedWhite:0.97 alpha:1.0];

  // Turn off aliasing so it's nice and crisp.
  NSGraphicsContext *context = [NSGraphicsContext currentContext];
  BOOL alias = [context shouldAntialias];
  [context setShouldAntialias:NO];

  // Handle bidirectional locales properly.
  CGFloat inset = [self isRToL] ? NSMaxX(bounds) - kBaseInset - kDragHandleWidth
                                : kBaseInset;

  NSPoint top = NSMakePoint(inset, NSMidY(bounds) - kHeight / 2.0);
  NSPoint bottom = NSMakePoint(inset, top.y + kHeight);

  path = [NSBezierPath bezierPath];
  [path moveToPoint:top];
  [path lineToPoint:bottom];
  [dark setStroke];
  [path stroke];

  top.x += 1;
  bottom.x += 1;
  path = [NSBezierPath bezierPath];
  [path moveToPoint:top];
  [path lineToPoint:bottom];
  [light setStroke];
  [path stroke];

  top.x += 2;
  bottom.x += 2;
  path = [NSBezierPath bezierPath];
  [path moveToPoint:top];
  [path lineToPoint:bottom];
  [dark setStroke];
  [path stroke];

  top.x += 1;
  bottom.x += 1;
  path = [NSBezierPath bezierPath];
  [path moveToPoint:top];
  [path lineToPoint:bottom];
  [light setStroke];
  [path stroke];

  [context setShouldAntialias:alias];
}

@end
