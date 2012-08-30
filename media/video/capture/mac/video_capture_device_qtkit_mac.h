// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// VideoCaptureDeviceQTKit implements all QTKit related code for
// communicating with a QTKit capture device.

#ifndef MEDIA_VIDEO_CAPTURE_MAC_VIDEO_CAPTURE_DEVICE_MAC_QTKIT_H_
#define MEDIA_VIDEO_CAPTURE_MAC_VIDEO_CAPTURE_DEVICE_MAC_QTKIT_H_

#import <Foundation/Foundation.h>

namespace media {
  class VideoCaptureDeviceMac;
}

@class QTCaptureDecompressedVideoOutput;
@class QTCaptureDeviceInput;
@class QTCaptureSession;

@interface VideoCaptureDeviceQTKit : NSObject {
 @private
  // Settings.
  int frameRate_;
  int frameWidth_;
  int frameHeight_;

  media::VideoCaptureDeviceMac *frameReceiver_;

  // QTKit variables.
  QTCaptureSession *captureSession_;
  QTCaptureDeviceInput *captureDeviceInput_;
  QTCaptureDecompressedVideoOutput *captureDecompressedOutput_;
}

// Returns a dictionary of capture devices with friendly name and unique id.
+ (NSDictionary *)deviceNames;

// Initializes the instance and registers the frame receiver.
- (id)initWithFrameReceiver:(media::VideoCaptureDeviceMac *)frameReceiver;

// Sets which capture device to use. Returns YES on sucess, NO otherwise.
- (BOOL)setCaptureDevice:(NSString *)deviceId;

// Configures the capture properties.
- (BOOL)setCaptureHeight:(int)height width:(int)width frameRate:(int)frameRate;

// Start video capturing. Returns YES on sucess, NO otherwise.
- (BOOL)startCapture;

// Stops video capturing.
- (void)stopCapture;

@end

#endif  // MEDIA_VIDEO_CAPTURE_MAC_VIDEO_CAPTURE_DEVICE_MAC_QTKIT_H_
