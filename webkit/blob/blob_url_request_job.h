// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BLOB_BLOB_URL_REQUEST_JOB_H_
#define WEBKIT_BLOB_BLOB_URL_REQUEST_JOB_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/platform_file.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request_job.h"
#include "webkit/blob/blob_data.h"
#include "webkit/blob/blob_export.h"

namespace base {
class MessageLoopProxy;
struct PlatformFileInfo;
}

namespace net {
class DrainableIOBuffer;
class IOBuffer;
}

namespace webkit_blob {

class LocalFileStreamReader;

// A request job that handles reading blob URLs.
class BLOB_EXPORT BlobURLRequestJob : public net::URLRequestJob {
 public:
  BlobURLRequestJob(net::URLRequest* request,
                    net::NetworkDelegate* network_delegate,
                    BlobData* blob_data,
                    base::MessageLoopProxy* resolving_message_loop_proxy);

  // net::URLRequestJob methods.
  virtual void Start() OVERRIDE;
  virtual void Kill() OVERRIDE;
  virtual bool ReadRawData(net::IOBuffer* buf,
                           int buf_size,
                           int* bytes_read) OVERRIDE;
  virtual bool GetMimeType(std::string* mime_type) const OVERRIDE;
  virtual void GetResponseInfo(net::HttpResponseInfo* info) OVERRIDE;
  virtual int GetResponseCode() const OVERRIDE;
  virtual void SetExtraRequestHeaders(
      const net::HttpRequestHeaders& headers) OVERRIDE;

 protected:
  virtual ~BlobURLRequestJob();

 private:
  typedef std::map<size_t, LocalFileStreamReader*> IndexToReaderMap;

  // For preparing for read: get the size, apply the range and perform seek.
  void DidStart();
  void CountSize();
  void DidCountSize(int error);
  void DidGetFileItemLength(size_t index, int64 result);
  void Seek(int64 offset);

  // For reading the blob.
  bool ReadLoop(int* bytes_read);
  bool ReadItem();
  void AdvanceItem();
  void AdvanceBytesRead(int result);
  bool ReadBytesItem(const BlobData::Item& item, int bytes_to_read);
  bool ReadFileItem(LocalFileStreamReader* reader, int bytes_to_read);

  void DidReadFile(int result);
  void DeleteCurrentFileReader();

  int ComputeBytesToRead() const;
  int BytesReadCompleted();

  void NotifySuccess();
  void NotifyFailure(int);
  void HeadersCompleted(int status_code, const std::string& status_txt);

  // Returns a LocalFileStreamReader for a blob item at |index|.
  // If the item at |index| is not of TYPE_FILE this returns NULL.
  LocalFileStreamReader* GetFileStreamReader(size_t index);

  base::WeakPtrFactory<BlobURLRequestJob> weak_factory_;
  scoped_refptr<BlobData> blob_data_;
  scoped_refptr<base::MessageLoopProxy> file_thread_proxy_;
  std::vector<int64> item_length_list_;
  int64 total_size_;
  int64 remaining_bytes_;
  int pending_get_file_info_count_;
  IndexToReaderMap index_to_reader_;
  size_t current_item_index_;
  int64 current_item_offset_;
  scoped_refptr<net::DrainableIOBuffer> read_buf_;
  bool error_;
  bool headers_set_;
  bool byte_range_set_;
  net::HttpByteRange byte_range_;
  scoped_ptr<net::HttpResponseInfo> response_info_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLRequestJob);
};

}  // namespace webkit_blob

#endif  // WEBKIT_BLOB_BLOB_URL_REQUEST_JOB_H_
