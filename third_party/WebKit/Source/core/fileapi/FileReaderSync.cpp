/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/fileapi/FileReaderSync.h"

#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/fileapi/Blob.h"
#include "core/fileapi/FileError.h"
#include "core/fileapi/FileReaderLoader.h"
#include "wtf/ArrayBuffer.h"
#include "wtf/PassRefPtr.h"

namespace WebCore {

FileReaderSync::FileReaderSync()
{
    ScriptWrappable::init(this);
}

PassRefPtr<ArrayBuffer> FileReaderSync::readAsArrayBuffer(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionState& es)
{
    if (!blob) {
        es.throwDOMException(NotFoundError, FileError::notFoundErrorMessage);
        return 0;
    }

    FileReaderLoader loader(FileReaderLoader::ReadAsArrayBuffer, 0);
    startLoading(scriptExecutionContext, loader, *blob, es);

    return loader.arrayBufferResult();
}

String FileReaderSync::readAsBinaryString(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionState& es)
{
    if (!blob) {
        es.throwDOMException(NotFoundError, FileError::notFoundErrorMessage);
        return String();
    }

    FileReaderLoader loader(FileReaderLoader::ReadAsBinaryString, 0);
    startLoading(scriptExecutionContext, loader, *blob, es);
    return loader.stringResult();
}

String FileReaderSync::readAsText(ScriptExecutionContext* scriptExecutionContext, Blob* blob, const String& encoding, ExceptionState& es)
{
    if (!blob) {
        es.throwDOMException(NotFoundError, FileError::notFoundErrorMessage);
        return String();
    }

    FileReaderLoader loader(FileReaderLoader::ReadAsText, 0);
    loader.setEncoding(encoding);
    startLoading(scriptExecutionContext, loader, *blob, es);
    return loader.stringResult();
}

String FileReaderSync::readAsDataURL(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionState& es)
{
    if (!blob) {
        es.throwDOMException(NotFoundError, FileError::notFoundErrorMessage);
        return String();
    }

    FileReaderLoader loader(FileReaderLoader::ReadAsDataURL, 0);
    loader.setDataType(blob->type());
    startLoading(scriptExecutionContext, loader, *blob, es);
    return loader.stringResult();
}

void FileReaderSync::startLoading(ScriptExecutionContext* scriptExecutionContext, FileReaderLoader& loader, const Blob& blob, ExceptionState& es)
{
    loader.start(scriptExecutionContext, blob);
    if (loader.errorCode())
        FileError::throwDOMException(es, loader.errorCode());
}

} // namespace WebCore
