// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_message_process_host.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/extensions/api/messaging/native_messaging_host_manifest.h"
#include "chrome/browser/extensions/api/messaging/native_process_launcher.h"
#include "chrome/common/chrome_version_info.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/constants.h"
#include "extensions/common/features/feature.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "url/gurl.h"

namespace {

// Maximum message size in bytes for messages received from Native Messaging
// hosts. Message size is limited mainly to prevent Chrome from crashing when
// native application misbehaves (e.g. starts writing garbage to the pipe).
const size_t kMaximumMessageSize = 1024 * 1024;

// Message header contains 4-byte integer size of the message.
const size_t kMessageHeaderSize = 4;

// Size of the buffer to be allocated for each read.
const size_t kReadBufferSize = 4096;

}  // namespace

namespace extensions {

NativeMessageProcessHost::NativeMessageProcessHost(
    const std::string& source_extension_id,
    const std::string& native_host_name,
    scoped_ptr<NativeProcessLauncher> launcher)
    : client_(NULL),
      source_extension_id_(source_extension_id),
      native_host_name_(native_host_name),
      launcher_(launcher.Pass()),
      closed_(false),
#if defined(OS_POSIX)
      read_file_(-1),
#endif
      read_pending_(false),
      write_pending_(false),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  task_runner_ = content::BrowserThread::GetMessageLoopProxyForThread(
      content::BrowserThread::IO);
}

NativeMessageProcessHost::~NativeMessageProcessHost() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (process_.IsValid()) {
    // Kill the host process if necessary to make sure we don't leave zombies.
    // On OSX base::EnsureProcessTerminated() may block, so we have to post a
    // task on the blocking pool.
#if defined(OS_MACOSX)
    content::BrowserThread::PostBlockingPoolTask(
        FROM_HERE,
        base::Bind(&base::EnsureProcessTerminated, Passed(&process_)));
#else
    base::EnsureProcessTerminated(process_.Pass());
#endif
  }
}

// static
scoped_ptr<NativeMessageHost> NativeMessageHost::Create(
    gfx::NativeView native_view,
    const std::string& source_extension_id,
    const std::string& native_host_name,
    bool allow_user_level,
    std::string* error_message) {
  return NativeMessageProcessHost::CreateWithLauncher(
      source_extension_id,
      native_host_name,
      NativeProcessLauncher::CreateDefault(allow_user_level, native_view));
}

// static
scoped_ptr<NativeMessageHost> NativeMessageProcessHost::CreateWithLauncher(
    const std::string& source_extension_id,
    const std::string& native_host_name,
    scoped_ptr<NativeProcessLauncher> launcher) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  scoped_ptr<NativeMessageHost> process(
      new NativeMessageProcessHost(source_extension_id,
                                   native_host_name,
                                   launcher.Pass()));

  return process.Pass();
}

void NativeMessageProcessHost::LaunchHostProcess() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  GURL origin(std::string(kExtensionScheme) + "://" + source_extension_id_);
  launcher_->Launch(origin, native_host_name_,
                    base::Bind(&NativeMessageProcessHost::OnHostProcessLaunched,
                               weak_factory_.GetWeakPtr()));
}

void NativeMessageProcessHost::OnHostProcessLaunched(
    NativeProcessLauncher::LaunchResult result,
    base::Process process,
    base::File read_file,
    base::File write_file) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  switch (result) {
    case NativeProcessLauncher::RESULT_INVALID_NAME:
      Close(kInvalidNameError);
      return;
    case NativeProcessLauncher::RESULT_NOT_FOUND:
      Close(kNotFoundError);
      return;
    case NativeProcessLauncher::RESULT_FORBIDDEN:
      Close(kForbiddenError);
      return;
    case NativeProcessLauncher::RESULT_FAILED_TO_START:
      Close(kFailedToStartError);
      return;
    case NativeProcessLauncher::RESULT_SUCCESS:
      break;
  }

  process_ = process.Pass();
#if defined(OS_POSIX)
  // This object is not the owner of the file so it should not keep an fd.
  read_file_ = read_file.GetPlatformFile();
#endif

  scoped_refptr<base::TaskRunner> task_runner(
      content::BrowserThread::GetBlockingPool()->
          GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));

  read_stream_.reset(new net::FileStream(read_file.Pass(), task_runner));
  write_stream_.reset(new net::FileStream(write_file.Pass(), task_runner));

  WaitRead();
  DoWrite();
}

void NativeMessageProcessHost::OnMessage(const std::string& json) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (closed_)
    return;

  // Allocate new buffer for the message.
  scoped_refptr<net::IOBufferWithSize> buffer =
      new net::IOBufferWithSize(json.size() + kMessageHeaderSize);

  // Copy size and content of the message to the buffer.
  static_assert(sizeof(uint32) == kMessageHeaderSize,
                "kMessageHeaderSize is incorrect");
  *reinterpret_cast<uint32*>(buffer->data()) = json.size();
  memcpy(buffer->data() + kMessageHeaderSize, json.data(), json.size());

  // Push new message to the write queue.
  write_queue_.push(buffer);

  // Send() may be called before the host process is started. In that case the
  // message will be written when OnHostProcessLaunched() is called. If it's
  // already started then write the message now.
  if (write_stream_)
    DoWrite();
}

void NativeMessageProcessHost::Start(Client* client) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!client_);
  client_ = client;
  // It's safe to use base::Unretained() here because NativeMessagePort always
  // deletes us on the IO thread.
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NativeMessageProcessHost::LaunchHostProcess,
                 weak_factory_.GetWeakPtr()));
}

scoped_refptr<base::SingleThreadTaskRunner>
NativeMessageProcessHost::task_runner() const {
  return task_runner_;
}

#if defined(OS_POSIX)
void NativeMessageProcessHost::OnFileCanReadWithoutBlocking(int fd) {
  DCHECK_EQ(fd, read_file_);
  DoRead();
}

void NativeMessageProcessHost::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED();
}
#endif  // !defined(OS_POSIX)

void NativeMessageProcessHost::ReadNowForTesting() {
  DoRead();
}

void NativeMessageProcessHost::WaitRead() {
  if (closed_)
    return;

  DCHECK(!read_pending_);

  // On POSIX FileStream::Read() uses blocking thread pool, so it's better to
  // wait for the file to become readable before calling DoRead(). Otherwise it
  // would always be consuming one thread in the thread pool. On Windows
  // FileStream uses overlapped IO, so that optimization isn't necessary there.
#if defined(OS_POSIX)
  base::MessageLoopForIO::current()->WatchFileDescriptor(
    read_file_, false /* persistent */,
    base::MessageLoopForIO::WATCH_READ, &read_watcher_, this);
#else  // defined(OS_POSIX)
  DoRead();
#endif  // defined(!OS_POSIX)
}

void NativeMessageProcessHost::DoRead() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  while (!closed_ && !read_pending_) {
    read_buffer_ = new net::IOBuffer(kReadBufferSize);
    int result =
        read_stream_->Read(read_buffer_.get(), kReadBufferSize,
                           base::Bind(&NativeMessageProcessHost::OnRead,
                                      weak_factory_.GetWeakPtr()));
    HandleReadResult(result);
  }
}

void NativeMessageProcessHost::OnRead(int result) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(read_pending_);
  read_pending_ = false;

  HandleReadResult(result);
  WaitRead();
}

void NativeMessageProcessHost::HandleReadResult(int result) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (closed_)
    return;

  if (result > 0) {
    ProcessIncomingData(read_buffer_->data(), result);
  } else if (result == net::ERR_IO_PENDING) {
    read_pending_ = true;
  } else if (result == 0 || result == net::ERR_CONNECTION_RESET) {
    // On Windows we get net::ERR_CONNECTION_RESET for a broken pipe, while on
    // Posix read() returns 0 in that case.
    Close(kNativeHostExited);
  } else {
    LOG(ERROR) << "Error when reading from Native Messaging host: " << result;
    Close(kHostInputOuputError);
  }
}

void NativeMessageProcessHost::ProcessIncomingData(
    const char* data, int data_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  incoming_data_.append(data, data_size);

  while (true) {
    if (incoming_data_.size() < kMessageHeaderSize)
      return;

    size_t message_size =
        *reinterpret_cast<const uint32*>(incoming_data_.data());

    if (message_size > kMaximumMessageSize) {
      LOG(ERROR) << "Native Messaging host tried sending a message that is "
                 << message_size << " bytes long.";
      Close(kHostInputOuputError);
      return;
    }

    if (incoming_data_.size() < message_size + kMessageHeaderSize)
      return;

    client_->PostMessageFromNativeHost(
        incoming_data_.substr(kMessageHeaderSize, message_size));

    incoming_data_.erase(0, kMessageHeaderSize + message_size);
  }
}

void NativeMessageProcessHost::DoWrite() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  while (!write_pending_ && !closed_) {
    if (!current_write_buffer_.get() ||
        !current_write_buffer_->BytesRemaining()) {
      if (write_queue_.empty())
        return;
      current_write_buffer_ = new net::DrainableIOBuffer(
          write_queue_.front().get(), write_queue_.front()->size());
      write_queue_.pop();
    }

    int result =
        write_stream_->Write(current_write_buffer_.get(),
                             current_write_buffer_->BytesRemaining(),
                             base::Bind(&NativeMessageProcessHost::OnWritten,
                                        weak_factory_.GetWeakPtr()));
    HandleWriteResult(result);
  }
}

void NativeMessageProcessHost::HandleWriteResult(int result) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (result <= 0) {
    if (result == net::ERR_IO_PENDING) {
      write_pending_ = true;
    } else {
      LOG(ERROR) << "Error when writing to Native Messaging host: " << result;
      Close(kHostInputOuputError);
    }
    return;
  }

  current_write_buffer_->DidConsume(result);
}

void NativeMessageProcessHost::OnWritten(int result) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  DCHECK(write_pending_);
  write_pending_ = false;

  HandleWriteResult(result);
  DoWrite();
}

void NativeMessageProcessHost::Close(const std::string& error_message) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!closed_) {
    closed_ = true;
    read_stream_.reset();
    write_stream_.reset();
    client_->CloseChannel(error_message);
  }
}

}  // namespace extensions
