// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/win/security_descriptor.h"

#include <sddl.h>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"

namespace remoting {

ScopedSd ConvertSddlToSd(const std::string& sddl) {
  PSECURITY_DESCRIPTOR raw_sd = NULL;
  ULONG length = 0;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
          UTF8ToUTF16(sddl).c_str(), SDDL_REVISION_1, &raw_sd, &length)) {
    return ScopedSd();
  }

  ScopedSd sd(length);
  memcpy(sd.get(), raw_sd, length);

  LocalFree(raw_sd);
  return sd.Pass();
}

// Converts a SID into a text string.
std::string ConvertSidToString(SID* sid) {
  char16* c_sid_string = NULL;
  if (!ConvertSidToStringSid(sid, &c_sid_string))
    return std::string();

  string16 sid_string(c_sid_string);
  LocalFree(c_sid_string);
  return UTF16ToUTF8(sid_string);
}

// Returns the logon SID of a token. Returns NULL if the token does not specify
// a logon SID or in case of an error.
ScopedSid GetLogonSid(HANDLE token) {
  DWORD length = 0;
  if (GetTokenInformation(token, TokenGroups, NULL, 0, &length) ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return ScopedSid();
  }

  TypedBuffer<TOKEN_GROUPS> groups(length);
  if (!GetTokenInformation(token, TokenGroups, groups.get(), length, &length))
    return ScopedSid();

  for (uint32 i = 0; i < groups->GroupCount; ++i) {
    if ((groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) ==
        SE_GROUP_LOGON_ID) {
      length = GetLengthSid(groups->Groups[i].Sid);
      ScopedSid logon_sid(length);
      if (!CopySid(length, logon_sid.get(), groups->Groups[i].Sid))
        return ScopedSid();

      return logon_sid.Pass();
    }
  }

  return ScopedSid();
}

bool MakeScopedAbsoluteSd(const ScopedSd& relative_sd,
                          ScopedSd* absolute_sd,
                          ScopedAcl* dacl,
                          ScopedSid* group,
                          ScopedSid* owner,
                          ScopedAcl* sacl) {
  // Get buffer sizes.
  DWORD absolute_sd_size = 0;
  DWORD dacl_size = 0;
  DWORD group_size = 0;
  DWORD owner_size = 0;
  DWORD sacl_size = 0;
  if (MakeAbsoluteSD(relative_sd.get(),
                     NULL,
                     &absolute_sd_size,
                     NULL,
                     &dacl_size,
                     NULL,
                     &sacl_size,
                     NULL,
                     &owner_size,
                     NULL,
                     &group_size) ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return false;
  }

  // Allocate buffers.
  ScopedSd local_absolute_sd(absolute_sd_size);
  ScopedAcl local_dacl(dacl_size);
  ScopedSid local_group(group_size);
  ScopedSid local_owner(owner_size);
  ScopedAcl local_sacl(sacl_size);

  // Do the conversion.
  if (!MakeAbsoluteSD(relative_sd.get(),
                      local_absolute_sd.get(),
                      &absolute_sd_size,
                      local_dacl.get(),
                      &dacl_size,
                      local_sacl.get(),
                      &sacl_size,
                      local_owner.get(),
                      &owner_size,
                      local_group.get(),
                      &group_size)) {
    return false;
  }

  absolute_sd->Swap(local_absolute_sd);
  dacl->Swap(local_dacl);
  group->Swap(local_group);
  owner->Swap(local_owner);
  sacl->Swap(local_sacl);
  return true;
}

}  // namespace remoting
