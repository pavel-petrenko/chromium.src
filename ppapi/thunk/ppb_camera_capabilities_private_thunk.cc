// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From private/ppb_camera_capabilities_private.idl,
//   modified Wed Aug 13 14:08:24 2014.

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_camera_capabilities_private.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_camera_capabilities_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_CameraCapabilities_Private::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateCameraCapabilitiesPrivate(instance);
}

PP_Bool IsCameraCapabilities(PP_Resource resource) {
  VLOG(4) << "PPB_CameraCapabilities_Private::IsCameraCapabilities()";
  EnterResource<PPB_CameraCapabilities_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

void GetSupportedPreviewSizes(PP_Resource capabilities,
                              int32_t* array_size,
                              struct PP_Size* preview_sizes[]) {
  VLOG(4) << "PPB_CameraCapabilities_Private::GetSupportedPreviewSizes()";
  EnterResource<PPB_CameraCapabilities_API> enter(capabilities, true);
  if (enter.failed())
    return;
  enter.object()->GetSupportedPreviewSizes(array_size, preview_sizes);
}

void GetSupportedJpegSizes(PP_Resource capabilities,
                           int32_t* array_size,
                           struct PP_Size* jpeg_sizes[]) {
  VLOG(4) << "PPB_CameraCapabilities_Private::GetSupportedJpegSizes()";
  EnterResource<PPB_CameraCapabilities_API> enter(capabilities, true);
  if (enter.failed())
    return;
  enter.object()->GetSupportedJpegSizes(array_size, jpeg_sizes);
}

const PPB_CameraCapabilities_Private_0_1
    g_ppb_cameracapabilities_private_thunk_0_1 = {
  &Create,
  &IsCameraCapabilities,
  &GetSupportedPreviewSizes,
  &GetSupportedJpegSizes
};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_CameraCapabilities_Private_0_1*
    GetPPB_CameraCapabilities_Private_0_1_Thunk() {
  return &g_ppb_cameracapabilities_private_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
