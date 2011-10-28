// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_METAFILE_IMPL_H_
#define PRINTING_METAFILE_IMPL_H_

#if defined(OS_WIN)
#include "printing/emf_win.h"
#elif defined(OS_MACOSX)
#include "printing/pdf_metafile_cg_mac.h"
#endif

#if !defined(OS_MACOSX) || defined(USE_SKIA)
#include "printing/pdf_metafile_skia.h"
#endif

namespace printing {

#if defined(OS_WIN)
typedef Emf NativeMetafile;
typedef PdfMetafileSkia PreviewMetafile;
#elif defined(OS_MACOSX)
#if defined(USE_SKIA)
typedef PdfMetafileSkia NativeMetafile;
typedef PdfMetafileSkia PreviewMetafile;
#else
typedef PdfMetafileCg NativeMetafile;
typedef PdfMetafileCg PreviewMetafile;
#endif
#elif defined(OS_POSIX)
typedef PdfMetafileSkia NativeMetafile;
typedef PdfMetafileSkia PreviewMetafile;
#endif

}  // namespace printing

#endif  // PRINTING_METAFILE_IMPL_H_
