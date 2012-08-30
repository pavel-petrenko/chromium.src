// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_SERIALIZED_STRUCTS_H_
#define PPAPI_PROXY_SERIALIZED_STRUCTS_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/shared_memory.h"
#include "build/build_config.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/proxy/serialized_var.h"
#include "ppapi/shared_impl/host_resource.h"

class Pickle;
struct PP_FontDescription_Dev;

namespace ppapi {
namespace proxy {

class Dispatcher;

// PP_FontDescript_Dev has to be redefined with a SerializedVar in place of
// the PP_Var used for the face name.
struct PPAPI_PROXY_EXPORT SerializedFontDescription {
  SerializedFontDescription();
  ~SerializedFontDescription();

  // Converts a PP_FontDescription_Dev to a SerializedFontDescription.
  //
  // If source_owns_ref is true, the reference owned by the
  // PP_FontDescription_Dev will be unchanged and the caller is responsible for
  // freeing it. When false, the SerializedFontDescription will take ownership
  // of the ref. This is the difference between serializing as an input value
  // (owns_ref = true) and an output value (owns_ref = true).
  void SetFromPPFontDescription(Dispatcher* dispatcher,
                                const PP_FontDescription_Dev& desc,
                                bool source_owns_ref);

  // Converts to a PP_FontDescription_Dev. The face name will have one ref
  // assigned to it on behalf of the caller.
  //
  // If dest_owns_ref is set, the resulting PP_FontDescription_Dev will keep a
  // reference to any strings we made on its behalf even when the
  // SerializedFontDescription goes away. When false, ownership of the ref will
  // stay with the SerializedFontDescription and the PP_FontDescription_Dev
  // will just refer to that one. This is the difference between deserializing
  // as an input value (owns_ref = false) and an output value (owns_ref = true).
  void SetToPPFontDescription(Dispatcher* dispatcher,
                              PP_FontDescription_Dev* desc,
                              bool dest_owns_ref) const;

  SerializedVar face;
  int32_t family;
  uint32_t size;
  int32_t weight;
  PP_Bool italic;
  PP_Bool small_caps;
  int32_t letter_spacing;
  int32_t word_spacing;
};

struct SerializedDirEntry {
  std::string name;
  bool is_dir;
};

struct PPBFlash_DrawGlyphs_Params {
  PPBFlash_DrawGlyphs_Params();
  ~PPBFlash_DrawGlyphs_Params();

  PP_Instance instance;
  ppapi::HostResource image_data;
  SerializedFontDescription font_desc;
  uint32_t color;
  PP_Point position;
  PP_Rect clip;
  float transformation[3][3];
  PP_Bool allow_subpixel_aa;
  std::vector<uint16_t> glyph_indices;
  std::vector<PP_Point> glyph_advances;
};

struct PPBURLLoader_UpdateProgress_Params {
  PP_Instance instance;
  ppapi::HostResource resource;
  int64_t bytes_sent;
  int64_t total_bytes_to_be_sent;
  int64_t bytes_received;
  int64_t total_bytes_to_be_received;
};

struct PPPVideoCapture_Buffer {
  ppapi::HostResource resource;
  uint32_t size;
  base::SharedMemoryHandle handle;
};

// We put all our handles in a unified structure to make it easy to translate
// them in NaClIPCAdapter for use in NaCl.
class PPAPI_PROXY_EXPORT SerializedHandle {
 public:
  enum Type { INVALID, SHARED_MEMORY, SOCKET };
  struct Header {
    Header() : type(INVALID), size(0) {}
    Header(Type type_arg, uint32_t size_arg)
        : type(type_arg), size(size_arg) {
    }
    Type type;
    uint32_t size;
  };

  SerializedHandle();
  // Create an invalid handle of the given type.
  explicit SerializedHandle(Type type);

  // Create a shared memory handle.
  SerializedHandle(const base::SharedMemoryHandle& handle, uint32_t size);

  // Create a socket handle.
  // TODO(dmichael): If we have other ways to use FDs later, this would be
  //                 ambiguous.
  explicit SerializedHandle(
      const IPC::PlatformFileForTransit& socket_descriptor);

  Type type() const { return type_; }
  bool is_shmem() const { return type_ == SHARED_MEMORY; }
  bool is_socket() const { return type_ == SOCKET; }
  const base::SharedMemoryHandle& shmem() const {
    DCHECK(is_shmem());
    return shm_handle_;
  }
  uint32_t size() const {
    DCHECK(is_shmem());
    return size_;
  }
  const IPC::PlatformFileForTransit& descriptor() const {
    DCHECK(is_socket());
    return descriptor_;
  }
  void set_shmem(const base::SharedMemoryHandle& handle, uint32_t size) {
    type_ = SHARED_MEMORY;
    shm_handle_ = handle;
    size_ = size;

    descriptor_ = IPC::InvalidPlatformFileForTransit();
  }
  void set_socket(const IPC::PlatformFileForTransit& socket) {
    type_ = SOCKET;
    descriptor_ = socket;

    shm_handle_ = base::SharedMemory::NULLHandle();
    size_ = 0;
  }
  void set_null_shmem() {
    set_shmem(base::SharedMemory::NULLHandle(), 0);
  }
  void set_null_socket() {
    set_socket(IPC::InvalidPlatformFileForTransit());
  }
  bool IsHandleValid() const;

  Header header() const {
    return Header(type_, size_);
  }

  // Write/Read a Header, which contains all the data except the handle. This
  // allows us to write the handle in a platform-specific way, as is necessary
  // in NaClIPCAdapter to share handles with NaCl from Windows.
  static bool WriteHeader(const Header& hdr, Pickle* pickle);
  static bool ReadHeader(PickleIterator* iter, Header* hdr);

 private:
  // The kind of handle we're holding.
  Type type_;

  // We hold more members than we really need; we can't easily use a union,
  // because we hold non-POD types. But these types are pretty light-weight. If
  // we add more complex things later, we should come up with a more memory-
  // efficient strategy.
  // These are valid if type == SHARED_MEMORY.
  base::SharedMemoryHandle shm_handle_;
  uint32_t size_;

  // This is valid if type == SOCKET.
  IPC::PlatformFileForTransit descriptor_;
};

// TODO(tomfinegan): This is identical to PPPVideoCapture_Buffer, maybe replace
// both with a single type?
struct PPPDecryptor_Buffer {
  ppapi::HostResource resource;
  uint32_t size;
  base::SharedMemoryHandle handle;
};

#if defined(OS_WIN)
typedef HANDLE ImageHandle;
#elif defined(OS_MACOSX) || defined(OS_ANDROID)
typedef base::SharedMemoryHandle ImageHandle;
#else
// On X Windows this is a SysV shared memory key.
typedef int ImageHandle;
#endif

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_SERIALIZED_STRUCTS_H_
