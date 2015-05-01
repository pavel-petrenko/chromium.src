// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/test_url_fetcher_factory.h"

#include <string>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "net/base/host_port_pair.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_fetcher_impl.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_status.h"

namespace net {

ScopedURLFetcherFactory::ScopedURLFetcherFactory(
    URLFetcherFactory* factory) {
  DCHECK(!URLFetcherImpl::factory());
  URLFetcherImpl::set_factory(factory);
}

ScopedURLFetcherFactory::~ScopedURLFetcherFactory() {
  DCHECK(URLFetcherImpl::factory());
  URLFetcherImpl::set_factory(NULL);
}

TestURLFetcher::TestURLFetcher(int id,
                               const GURL& url,
                               URLFetcherDelegate* d)
    : owner_(NULL),
      id_(id),
      original_url_(url),
      delegate_(d),
      delegate_for_tests_(NULL),
      did_receive_last_chunk_(false),
      fake_load_flags_(0),
      fake_response_code_(-1),
      fake_response_destination_(STRING),
      fake_was_fetched_via_proxy_(false),
      fake_max_retries_(0) {
  CHECK(original_url_.is_valid());
}

TestURLFetcher::~TestURLFetcher() {
  if (delegate_for_tests_)
    delegate_for_tests_->OnRequestEnd(id_);
  if (owner_)
    owner_->RemoveFetcherFromMap(id_);
}

void TestURLFetcher::SetUploadData(const std::string& upload_content_type,
                                   const std::string& upload_content) {
  upload_content_type_ = upload_content_type;
  upload_data_ = upload_content;
}

void TestURLFetcher::SetUploadFilePath(
    const std::string& upload_content_type,
    const base::FilePath& file_path,
    uint64 range_offset,
    uint64 range_length,
    scoped_refptr<base::TaskRunner> file_task_runner) {
  upload_file_path_ = file_path;
}

void TestURLFetcher::SetUploadStreamFactory(
    const std::string& upload_content_type,
    const CreateUploadStreamCallback& factory) {
}

void TestURLFetcher::SetChunkedUpload(const std::string& upload_content_type) {
}

void TestURLFetcher::AppendChunkToUpload(const std::string& data,
                                         bool is_last_chunk) {
  DCHECK(!did_receive_last_chunk_);
  did_receive_last_chunk_ = is_last_chunk;
  chunks_.push_back(data);
  if (delegate_for_tests_)
    delegate_for_tests_->OnChunkUpload(id_);
}

void TestURLFetcher::SetLoadFlags(int load_flags) {
  fake_load_flags_= load_flags;
}

int TestURLFetcher::GetLoadFlags() const {
  return fake_load_flags_;
}

void TestURLFetcher::SetReferrer(const std::string& referrer) {
}

void TestURLFetcher::SetReferrerPolicy(
    URLRequest::ReferrerPolicy referrer_policy) {
}

void TestURLFetcher::SetExtraRequestHeaders(
    const std::string& extra_request_headers) {
  fake_extra_request_headers_.Clear();
  fake_extra_request_headers_.AddHeadersFromString(extra_request_headers);
}

void TestURLFetcher::AddExtraRequestHeader(const std::string& header_line) {
  fake_extra_request_headers_.AddHeaderFromString(header_line);
}

void TestURLFetcher::SetRequestContext(
    URLRequestContextGetter* request_context_getter) {
}

void TestURLFetcher::SetFirstPartyForCookies(
    const GURL& first_party_for_cookies) {
}

void TestURLFetcher::SetURLRequestUserData(
    const void* key,
    const CreateDataCallback& create_data_callback) {
}

void TestURLFetcher::SetStopOnRedirect(bool stop_on_redirect) {
}

void TestURLFetcher::SetAutomaticallyRetryOn5xx(bool retry) {
}

void TestURLFetcher::SetMaxRetriesOn5xx(int max_retries) {
  fake_max_retries_ = max_retries;
}

int TestURLFetcher::GetMaxRetriesOn5xx() const {
  return fake_max_retries_;
}

base::TimeDelta TestURLFetcher::GetBackoffDelay() const {
  return fake_backoff_delay_;
}

void TestURLFetcher::SetAutomaticallyRetryOnNetworkChanges(int max_retries) {
}

void TestURLFetcher::SaveResponseToFileAtPath(
    const base::FilePath& file_path,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
  SetResponseFilePath(file_path);
  // Asynchronous IO is not supported, so file_task_runner is ignored.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  const size_t written_bytes = base::WriteFile(
      file_path, fake_response_string_.c_str(), fake_response_string_.size());
  DCHECK_EQ(written_bytes, fake_response_string_.size());
}

void TestURLFetcher::SaveResponseToTemporaryFile(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
}

void TestURLFetcher::SaveResponseWithWriter(
    scoped_ptr<URLFetcherResponseWriter> response_writer) {
  // In class URLFetcherCore this method is called by all three:
  // GetResponseAsString() / SaveResponseToFileAtPath() /
  // SaveResponseToTemporaryFile(). But here (in TestURLFetcher), this method
  // is never used by any of these three methods. So, file writing is expected
  // to be done in SaveResponseToFileAtPath(), and this method supports only
  // URLFetcherStringWriter (for testing of this method only).
  if (fake_response_destination_ == STRING) {
    response_writer_ = response_writer.Pass();
    int response = response_writer_->Initialize(CompletionCallback());
    // The TestURLFetcher doesn't handle asynchronous writes.
    DCHECK_EQ(OK, response);

    scoped_refptr<IOBuffer> buffer(new StringIOBuffer(fake_response_string_));
    response = response_writer_->Write(buffer.get(),
                                       fake_response_string_.size(),
                                       CompletionCallback());
    DCHECK_EQ(static_cast<int>(fake_response_string_.size()), response);
    response = response_writer_->Finish(CompletionCallback());
    DCHECK_EQ(OK, response);
  } else if (fake_response_destination_ == TEMP_FILE) {
    // SaveResponseToFileAtPath() should be called instead of this method to
    // save file. Asynchronous file writing using URLFetcherFileWriter is not
    // supported.
    NOTIMPLEMENTED();
  } else {
    NOTREACHED();
  }
}

HttpResponseHeaders* TestURLFetcher::GetResponseHeaders() const {
  return fake_response_headers_.get();
}

HostPortPair TestURLFetcher::GetSocketAddress() const {
  NOTIMPLEMENTED();
  return HostPortPair();
}

bool TestURLFetcher::WasFetchedViaProxy() const {
  return fake_was_fetched_via_proxy_;
}

void TestURLFetcher::Start() {
  // Overriden to do nothing. It is assumed the caller will notify the delegate.
  if (delegate_for_tests_)
    delegate_for_tests_->OnRequestStart(id_);
}

const GURL& TestURLFetcher::GetOriginalURL() const {
  return original_url_;
}

const GURL& TestURLFetcher::GetURL() const {
  return fake_url_;
}

const URLRequestStatus& TestURLFetcher::GetStatus() const {
  return fake_status_;
}

int TestURLFetcher::GetResponseCode() const {
  return fake_response_code_;
}

const ResponseCookies& TestURLFetcher::GetCookies() const {
  return fake_cookies_;
}

void TestURLFetcher::ReceivedContentWasMalformed() {
}

bool TestURLFetcher::GetResponseAsString(
    std::string* out_response_string) const {
  if (fake_response_destination_ != STRING)
    return false;

  *out_response_string = fake_response_string_;
  return true;
}

bool TestURLFetcher::GetResponseAsFilePath(
    bool take_ownership, base::FilePath* out_response_path) const {
  if (fake_response_destination_ != TEMP_FILE)
    return false;

  *out_response_path = fake_response_file_path_;
  return true;
}

void TestURLFetcher::GetExtraRequestHeaders(
    HttpRequestHeaders* headers) const {
  *headers = fake_extra_request_headers_;
}

void TestURLFetcher::set_status(const URLRequestStatus& status) {
  fake_status_ = status;
}

void TestURLFetcher::set_was_fetched_via_proxy(bool flag) {
  fake_was_fetched_via_proxy_ = flag;
}

void TestURLFetcher::set_response_headers(
    scoped_refptr<HttpResponseHeaders> headers) {
  fake_response_headers_ = headers;
}

void TestURLFetcher::set_backoff_delay(base::TimeDelta backoff_delay) {
  fake_backoff_delay_ = backoff_delay;
}

void TestURLFetcher::SetDelegateForTests(DelegateForTests* delegate_for_tests) {
  delegate_for_tests_ = delegate_for_tests;
}

void TestURLFetcher::SetResponseString(const std::string& response) {
  fake_response_destination_ = STRING;
  fake_response_string_ = response;
}

void TestURLFetcher::SetResponseFilePath(const base::FilePath& path) {
  fake_response_destination_ = TEMP_FILE;
  fake_response_file_path_ = path;
}

TestURLFetcherFactory::TestURLFetcherFactory()
    : ScopedURLFetcherFactory(this),
      delegate_for_tests_(NULL),
      remove_fetcher_on_delete_(false) {
}

TestURLFetcherFactory::~TestURLFetcherFactory() {}

scoped_ptr<URLFetcher> TestURLFetcherFactory::CreateURLFetcher(
    int id,
    const GURL& url,
    URLFetcher::RequestType request_type,
    URLFetcherDelegate* d) {
  TestURLFetcher* fetcher = new TestURLFetcher(id, url, d);
  if (remove_fetcher_on_delete_)
    fetcher->set_owner(this);
  fetcher->SetDelegateForTests(delegate_for_tests_);
  fetchers_[id] = fetcher;
  return scoped_ptr<URLFetcher>(fetcher);
}

TestURLFetcher* TestURLFetcherFactory::GetFetcherByID(int id) const {
  Fetchers::const_iterator i = fetchers_.find(id);
  return i == fetchers_.end() ? NULL : i->second;
}

void TestURLFetcherFactory::RemoveFetcherFromMap(int id) {
  Fetchers::iterator i = fetchers_.find(id);
  DCHECK(i != fetchers_.end());
  fetchers_.erase(i);
}

void TestURLFetcherFactory::SetDelegateForTests(
    TestURLFetcherDelegateForTests* delegate_for_tests) {
  delegate_for_tests_ = delegate_for_tests;
}

FakeURLFetcher::FakeURLFetcher(const GURL& url,
                               URLFetcherDelegate* d,
                               const std::string& response_data,
                               HttpStatusCode response_code,
                               URLRequestStatus::Status status)
    : TestURLFetcher(0, url, d),
      weak_factory_(this) {
  Error error = OK;
  switch(status) {
    case URLRequestStatus::SUCCESS:
      // |error| is initialized to OK.
      break;
    case URLRequestStatus::IO_PENDING:
      error = ERR_IO_PENDING;
      break;
    case URLRequestStatus::CANCELED:
      error = ERR_ABORTED;
      break;
    case URLRequestStatus::FAILED:
      error = ERR_FAILED;
      break;
  }
  set_status(URLRequestStatus(status, error));
  set_response_code(response_code);
  SetResponseString(response_data);
}

FakeURLFetcher::~FakeURLFetcher() {}

void FakeURLFetcher::Start() {
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&FakeURLFetcher::RunDelegate, weak_factory_.GetWeakPtr()));
}

void FakeURLFetcher::RunDelegate() {
  delegate()->OnURLFetchComplete(this);
}

const GURL& FakeURLFetcher::GetURL() const {
  return TestURLFetcher::GetOriginalURL();
}

FakeURLFetcherFactory::FakeURLFetcherFactory(
    URLFetcherFactory* default_factory)
    : ScopedURLFetcherFactory(this),
      creator_(base::Bind(&DefaultFakeURLFetcherCreator)),
      default_factory_(default_factory) {
}

FakeURLFetcherFactory::FakeURLFetcherFactory(
    URLFetcherFactory* default_factory,
    const FakeURLFetcherCreator& creator)
    : ScopedURLFetcherFactory(this),
      creator_(creator),
      default_factory_(default_factory) {
}

scoped_ptr<FakeURLFetcher> FakeURLFetcherFactory::DefaultFakeURLFetcherCreator(
      const GURL& url,
      URLFetcherDelegate* delegate,
      const std::string& response_data,
      HttpStatusCode response_code,
      URLRequestStatus::Status status) {
  return scoped_ptr<FakeURLFetcher>(
      new FakeURLFetcher(url, delegate, response_data, response_code, status));
}

FakeURLFetcherFactory::~FakeURLFetcherFactory() {}

scoped_ptr<URLFetcher> FakeURLFetcherFactory::CreateURLFetcher(
    int id,
    const GURL& url,
    URLFetcher::RequestType request_type,
    URLFetcherDelegate* d) {
  FakeResponseMap::const_iterator it = fake_responses_.find(url);
  if (it == fake_responses_.end()) {
    if (default_factory_ == NULL) {
      // If we don't have a baked response for that URL we return NULL.
      DLOG(ERROR) << "No baked response for URL: " << url.spec();
      return NULL;
    } else {
      return default_factory_->CreateURLFetcher(id, url, request_type, d);
    }
  }

  scoped_ptr<URLFetcher> fake_fetcher =
      creator_.Run(url, d, it->second.response_data, it->second.response_code,
                   it->second.status);
  return fake_fetcher;
}

void FakeURLFetcherFactory::SetFakeResponse(
    const GURL& url,
    const std::string& response_data,
    HttpStatusCode response_code,
    URLRequestStatus::Status status) {
  // Overwrite existing URL if it already exists.
  FakeURLResponse response;
  response.response_data = response_data;
  response.response_code = response_code;
  response.status = status;
  fake_responses_[url] = response;
}

void FakeURLFetcherFactory::ClearFakeResponses() {
  fake_responses_.clear();
}

URLFetcherImplFactory::URLFetcherImplFactory() {}

URLFetcherImplFactory::~URLFetcherImplFactory() {}

scoped_ptr<URLFetcher> URLFetcherImplFactory::CreateURLFetcher(
    int id,
    const GURL& url,
    URLFetcher::RequestType request_type,
    URLFetcherDelegate* d) {
  return scoped_ptr<URLFetcher>(new URLFetcherImpl(url, request_type, d));
}

}  // namespace net
