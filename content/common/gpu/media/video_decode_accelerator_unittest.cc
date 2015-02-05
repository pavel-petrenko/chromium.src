// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The bulk of this file is support code; sorry about that.  Here's an overview
// to hopefully help readers of this code:
// - RenderingHelper is charged with interacting with X11/{EGL/GLES2,GLX/GL} or
//   Win/EGL.
// - ClientState is an enum for the state of the decode client used by the test.
// - ClientStateNotification is a barrier abstraction that allows the test code
//   to be written sequentially and wait for the decode client to see certain
//   state transitions.
// - GLRenderingVDAClient is a VideoDecodeAccelerator::Client implementation
// - Finally actual TEST cases are at the bottom of this file, using the above
//   infrastructure.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <deque>
#include <map>

// Include gtest.h out of order because <X11/X.h> #define's Bool & None, which
// gtest uses as struct names (inside a namespace).  This means that
// #include'ing gtest after anything that pulls in X.h fails to compile.
// This is http://code.google.com/p/googletest/issues/detail?id=371
#include "testing/gtest/include/gtest/gtest.h"

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/md5.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/process/process_handle.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringize_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "content/common/gpu/media/rendering_helper.h"
#include "content/common/gpu/media/video_accelerator_unittest_helpers.h"
#include "content/public/common/content_switches.h"
#include "media/filters/h264_parser.h"
#include "ui/gfx/codec/png_codec.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "content/common/gpu/media/dxva_video_decode_accelerator.h"
#elif defined(OS_CHROMEOS)
#if defined(ARCH_CPU_ARMEL) && defined(USE_LIBV4L2)
#include "content/common/gpu/media/v4l2_slice_video_decode_accelerator.h"
#endif  // defined(ARCH_CPU_ARMEL)
#if defined(ARCH_CPU_ARMEL) || (defined(USE_OZONE) && defined(USE_V4L2_CODEC))
#include "content/common/gpu/media/v4l2_video_decode_accelerator.h"
#include "content/common/gpu/media/v4l2_video_device.h"
#endif
#if defined(ARCH_CPU_X86_FAMILY)
#include "content/common/gpu/media/vaapi_video_decode_accelerator.h"
#include "content/common/gpu/media/vaapi_wrapper.h"
#endif  // defined(ARCH_CPU_X86_FAMILY)
#else
#error The VideoAccelerator tests are not supported on this platform.
#endif  // OS_WIN

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif  // defined(USE_OZONE)

using media::VideoDecodeAccelerator;

namespace content {
namespace {

// Values optionally filled in from flags; see main() below.
// The syntax of multiple test videos is:
//  test-video1;test-video2;test-video3
// where only the first video is required and other optional videos would be
// decoded by concurrent decoders.
// The syntax of each test-video is:
//  filename:width:height:numframes:numfragments:minFPSwithRender:minFPSnoRender
// where only the first field is required.  Value details:
// - |filename| must be an h264 Annex B (NAL) stream or an IVF VP8/9 stream.
// - |width| and |height| are in pixels.
// - |numframes| is the number of picture frames in the file.
// - |numfragments| NALU (h264) or frame (VP8/9) count in the stream.
// - |minFPSwithRender| and |minFPSnoRender| are minimum frames/second speeds
//   expected to be achieved with and without rendering to the screen, resp.
//   (the latter tests just decode speed).
// - |profile| is the media::VideoCodecProfile set during Initialization.
// An empty value for a numeric field means "ignore".
const base::FilePath::CharType* g_test_video_data =
    // FILE_PATH_LITERAL("test-25fps.vp8:320:240:250:250:50:175:11");
    FILE_PATH_LITERAL("test-25fps.h264:320:240:250:258:50:175:1");

// The file path of the test output log. This is used to communicate the test
// results to CrOS autotests. We can enable the log and specify the filename by
// the "--output_log" switch.
const base::FilePath::CharType* g_output_log = NULL;

// The value is set by the switch "--rendering_fps".
double g_rendering_fps = 60;

// The value is set by the switch "--rendering_warm_up".
int g_rendering_warm_up = 0;

// The value is set by the switch "--num_play_throughs". The video will play
// the specified number of times. In different test cases, we have different
// values for |num_play_throughs|. This setting will override the value. A
// special value "0" means no override.
int g_num_play_throughs = 0;

// Environment to store rendering thread.
class VideoDecodeAcceleratorTestEnvironment;
VideoDecodeAcceleratorTestEnvironment* g_env;

// Magic constants for differentiating the reasons for NotifyResetDone being
// called.
enum ResetPoint {
  // Reset() just after calling Decode() with a fragment containing config info.
  RESET_AFTER_FIRST_CONFIG_INFO = -4,
  START_OF_STREAM_RESET = -3,
  MID_STREAM_RESET = -2,
  END_OF_STREAM_RESET = -1
};

const int kMaxResetAfterFrameNum = 100;
const int kMaxFramesToDelayReuse = 64;
const base::TimeDelta kReuseDelay = base::TimeDelta::FromSeconds(1);
// Simulate WebRTC and call VDA::Decode 30 times per second.
const int kWebRtcDecodeCallsPerSecond = 30;

struct TestVideoFile {
  explicit TestVideoFile(base::FilePath::StringType file_name)
      : file_name(file_name),
        width(-1),
        height(-1),
        num_frames(-1),
        num_fragments(-1),
        min_fps_render(-1),
        min_fps_no_render(-1),
        profile(media::VIDEO_CODEC_PROFILE_UNKNOWN),
        reset_after_frame_num(END_OF_STREAM_RESET) {
  }

  base::FilePath::StringType file_name;
  int width;
  int height;
  int num_frames;
  int num_fragments;
  int min_fps_render;
  int min_fps_no_render;
  media::VideoCodecProfile profile;
  int reset_after_frame_num;
  std::string data_str;
};

const gfx::Size kThumbnailsPageSize(1600, 1200);
const gfx::Size kThumbnailSize(160, 120);
const int kMD5StringLength = 32;

// Read in golden MD5s for the thumbnailed rendering of this video
void ReadGoldenThumbnailMD5s(const TestVideoFile* video_file,
                             std::vector<std::string>* md5_strings) {
  base::FilePath filepath(video_file->file_name);
  filepath = filepath.AddExtension(FILE_PATH_LITERAL(".md5"));
  std::string all_md5s;
  base::ReadFileToString(filepath, &all_md5s);
  base::SplitString(all_md5s, '\n', md5_strings);
  // Check these are legitimate MD5s.
  for (std::vector<std::string>::iterator md5_string = md5_strings->begin();
      md5_string != md5_strings->end(); ++md5_string) {
      // Ignore the empty string added by SplitString
      if (!md5_string->length())
        continue;
      // Ignore comments
      if (md5_string->at(0) == '#')
        continue;

      CHECK_EQ(static_cast<int>(md5_string->length()),
               kMD5StringLength) << *md5_string;
      bool hex_only = std::count_if(md5_string->begin(),
                                    md5_string->end(), isxdigit) ==
                                    kMD5StringLength;
      CHECK(hex_only) << *md5_string;
  }
  CHECK_GE(md5_strings->size(), 1U) << all_md5s;
}

// State of the GLRenderingVDAClient below.  Order matters here as the test
// makes assumptions about it.
enum ClientState {
  CS_CREATED = 0,
  CS_DECODER_SET = 1,
  CS_INITIALIZED = 2,
  CS_FLUSHING = 3,
  CS_FLUSHED = 4,
  CS_RESETTING = 5,
  CS_RESET = 6,
  CS_ERROR = 7,
  CS_DESTROYED = 8,
  CS_MAX,  // Must be last entry.
};

// Initialize the GPU thread for rendering. We only need to setup once
// for all test cases.
class VideoDecodeAcceleratorTestEnvironment : public ::testing::Environment {
 public:
  VideoDecodeAcceleratorTestEnvironment()
      : rendering_thread_("GLRenderingVDAClientThread") {}

  void SetUp() override {
    rendering_thread_.Start();

    base::WaitableEvent done(false, false);
    rendering_thread_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&RenderingHelper::InitializeOneOff, &done));
    done.Wait();
  }

  void TearDown() override { rendering_thread_.Stop(); }

  scoped_refptr<base::SingleThreadTaskRunner> GetRenderingTaskRunner() const {
    return rendering_thread_.task_runner();
  }

 private:
  base::Thread rendering_thread_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecodeAcceleratorTestEnvironment);
};

// A helper class used to manage the lifetime of a Texture.
class TextureRef : public base::RefCounted<TextureRef> {
 public:
  TextureRef(uint32 texture_id, const base::Closure& no_longer_needed_cb)
      : texture_id_(texture_id), no_longer_needed_cb_(no_longer_needed_cb) {}

  int32 texture_id() const { return texture_id_; }

 private:
  friend class base::RefCounted<TextureRef>;
  ~TextureRef();

  uint32 texture_id_;
  base::Closure no_longer_needed_cb_;
};

TextureRef::~TextureRef() {
  base::ResetAndReturn(&no_longer_needed_cb_).Run();
}

// Client that can accept callbacks from a VideoDecodeAccelerator and is used by
// the TESTs below.
class GLRenderingVDAClient
    : public VideoDecodeAccelerator::Client,
      public base::SupportsWeakPtr<GLRenderingVDAClient> {
 public:
  // |window_id| the window_id of the client, which is used to identify the
  // rendering area in the |rendering_helper|.
  // Doesn't take ownership of |rendering_helper| or |note|, which must outlive
  // |*this|.
  // |num_play_throughs| indicates how many times to play through the video.
  // |reset_after_frame_num| can be a frame number >=0 indicating a mid-stream
  // Reset() should be done after that frame number is delivered, or
  // END_OF_STREAM_RESET to indicate no mid-stream Reset().
  // |delete_decoder_state| indicates when the underlying decoder should be
  // Destroy()'d and deleted and can take values: N<0: delete after -N Decode()
  // calls have been made, N>=0 means interpret as ClientState.
  // Both |reset_after_frame_num| & |delete_decoder_state| apply only to the
  // last play-through (governed by |num_play_throughs|).
  // |suppress_rendering| indicates GL rendering is supressed or not.
  // After |delay_reuse_after_frame_num| frame has been delivered, the client
  // will start delaying the call to ReusePictureBuffer() for kReuseDelay.
  // |decode_calls_per_second| is the number of VDA::Decode calls per second.
  // If |decode_calls_per_second| > 0, |num_in_flight_decodes| must be 1.
  GLRenderingVDAClient(size_t window_id,
                       RenderingHelper* rendering_helper,
                       ClientStateNotification<ClientState>* note,
                       const std::string& encoded_data,
                       int num_in_flight_decodes,
                       int num_play_throughs,
                       int reset_after_frame_num,
                       int delete_decoder_state,
                       int frame_width,
                       int frame_height,
                       media::VideoCodecProfile profile,
                       bool suppress_rendering,
                       int delay_reuse_after_frame_num,
                       int decode_calls_per_second,
                       bool render_as_thumbnails);
  virtual ~GLRenderingVDAClient();
  void CreateAndStartDecoder();

  // VideoDecodeAccelerator::Client implementation.
  // The heart of the Client.
  virtual void ProvidePictureBuffers(uint32 requested_num_of_buffers,
                                     const gfx::Size& dimensions,
                                     uint32 texture_target) override;
  virtual void DismissPictureBuffer(int32 picture_buffer_id) override;
  virtual void PictureReady(const media::Picture& picture) override;
  // Simple state changes.
  virtual void NotifyEndOfBitstreamBuffer(int32 bitstream_buffer_id) override;
  virtual void NotifyFlushDone() override;
  virtual void NotifyResetDone() override;
  virtual void NotifyError(VideoDecodeAccelerator::Error error) override;

  void OutputFrameDeliveryTimes(base::File* output);

  // Simple getters for inspecting the state of the Client.
  int num_done_bitstream_buffers() { return num_done_bitstream_buffers_; }
  int num_skipped_fragments() { return num_skipped_fragments_; }
  int num_queued_fragments() { return num_queued_fragments_; }
  int num_decoded_frames() { return num_decoded_frames_; }
  double frames_per_second();
  // Return the median of the decode time of all decoded frames.
  base::TimeDelta decode_time_median();
  bool decoder_deleted() { return !decoder_.get(); }

 private:
  typedef std::map<int32, scoped_refptr<TextureRef>> TextureRefMap;

  scoped_ptr<media::VideoDecodeAccelerator> CreateDXVAVDA();
  scoped_ptr<media::VideoDecodeAccelerator> CreateV4L2VDA();
  scoped_ptr<media::VideoDecodeAccelerator> CreateV4L2SliceVDA();
  scoped_ptr<media::VideoDecodeAccelerator> CreateVaapiVDA();

  void SetState(ClientState new_state);
  void FinishInitialization();
  void ReturnPicture(int32 picture_buffer_id);

  // Delete the associated decoder helper.
  void DeleteDecoder();

  // Compute & return the first encoded bytes (including a start frame) to send
  // to the decoder, starting at |start_pos| and returning one fragment. Skips
  // to the first decodable position.
  std::string GetBytesForFirstFragment(size_t start_pos, size_t* end_pos);
  // Compute & return the encoded bytes of next fragment to send to the decoder
  // (based on |start_pos|).
  std::string GetBytesForNextFragment(size_t start_pos, size_t* end_pos);
  // Helpers for GetBytesForNextFragment above.
  void GetBytesForNextNALU(size_t start_pos, size_t* end_pos);  // For h.264.
  std::string GetBytesForNextFrame(
      size_t start_pos, size_t* end_pos);  // For VP8/9.

  // Request decode of the next fragment in the encoded data.
  void DecodeNextFragment();

  size_t window_id_;
  RenderingHelper* rendering_helper_;
  gfx::Size frame_size_;
  std::string encoded_data_;
  const int num_in_flight_decodes_;
  int outstanding_decodes_;
  size_t encoded_data_next_pos_to_decode_;
  int next_bitstream_buffer_id_;
  ClientStateNotification<ClientState>* note_;
  scoped_ptr<VideoDecodeAccelerator> decoder_;
  scoped_ptr<base::WeakPtrFactory<VideoDecodeAccelerator> >
      weak_decoder_factory_;
  int remaining_play_throughs_;
  int reset_after_frame_num_;
  int delete_decoder_state_;
  ClientState state_;
  int num_skipped_fragments_;
  int num_queued_fragments_;
  int num_decoded_frames_;
  int num_done_bitstream_buffers_;
  base::TimeTicks initialize_done_ticks_;
  media::VideoCodecProfile profile_;
  GLenum texture_target_;
  bool suppress_rendering_;
  std::vector<base::TimeTicks> frame_delivery_times_;
  int delay_reuse_after_frame_num_;
  // A map from bitstream buffer id to the decode start time of the buffer.
  std::map<int, base::TimeTicks> decode_start_time_;
  // The decode time of all decoded frames.
  std::vector<base::TimeDelta> decode_time_;
  // The number of VDA::Decode calls per second. This is to simulate webrtc.
  int decode_calls_per_second_;
  bool render_as_thumbnails_;

  // A map of the textures that are currently active for the decoder, i.e.,
  // have been created via AssignPictureBuffers() and not dismissed via
  // DismissPictureBuffer(). The keys in the map are the IDs of the
  // corresponding picture buffers, and the values are TextureRefs to the
  // textures.
  TextureRefMap active_textures_;

  // A map of the textures that are still pending in the renderer.
  // We check this to ensure all frames are rendered before entering the
  // CS_RESET_State.
  TextureRefMap pending_textures_;

  int32 next_picture_buffer_id_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(GLRenderingVDAClient);
};

GLRenderingVDAClient::GLRenderingVDAClient(
    size_t window_id,
    RenderingHelper* rendering_helper,
    ClientStateNotification<ClientState>* note,
    const std::string& encoded_data,
    int num_in_flight_decodes,
    int num_play_throughs,
    int reset_after_frame_num,
    int delete_decoder_state,
    int frame_width,
    int frame_height,
    media::VideoCodecProfile profile,
    bool suppress_rendering,
    int delay_reuse_after_frame_num,
    int decode_calls_per_second,
    bool render_as_thumbnails)
    : window_id_(window_id),
      rendering_helper_(rendering_helper),
      frame_size_(frame_width, frame_height),
      encoded_data_(encoded_data),
      num_in_flight_decodes_(num_in_flight_decodes),
      outstanding_decodes_(0),
      encoded_data_next_pos_to_decode_(0),
      next_bitstream_buffer_id_(0),
      note_(note),
      remaining_play_throughs_(num_play_throughs),
      reset_after_frame_num_(reset_after_frame_num),
      delete_decoder_state_(delete_decoder_state),
      state_(CS_CREATED),
      num_skipped_fragments_(0),
      num_queued_fragments_(0),
      num_decoded_frames_(0),
      num_done_bitstream_buffers_(0),
      texture_target_(0),
      suppress_rendering_(suppress_rendering),
      delay_reuse_after_frame_num_(delay_reuse_after_frame_num),
      decode_calls_per_second_(decode_calls_per_second),
      render_as_thumbnails_(render_as_thumbnails),
      next_picture_buffer_id_(1) {
  CHECK_GT(num_in_flight_decodes, 0);
  CHECK_GT(num_play_throughs, 0);
  // |num_in_flight_decodes_| is unsupported if |decode_calls_per_second_| > 0.
  if (decode_calls_per_second_ > 0)
    CHECK_EQ(1, num_in_flight_decodes_);

  // Default to H264 baseline if no profile provided.
  profile_ = (profile != media::VIDEO_CODEC_PROFILE_UNKNOWN
                  ? profile
                  : media::H264PROFILE_BASELINE);
}

GLRenderingVDAClient::~GLRenderingVDAClient() {
  DeleteDecoder();  // Clean up in case of expected error.
  CHECK(decoder_deleted());
  SetState(CS_DESTROYED);
}

static bool DoNothingReturnTrue() { return true; }

scoped_ptr<media::VideoDecodeAccelerator>
GLRenderingVDAClient::CreateDXVAVDA() {
  scoped_ptr<media::VideoDecodeAccelerator> decoder;
#if defined(OS_WIN)
  if (base::win::GetVersion() >= base::win::VERSION_WIN7)
    decoder.reset(
        new DXVAVideoDecodeAccelerator(base::Bind(&DoNothingReturnTrue)));
#endif
  return decoder.Pass();
}

scoped_ptr<media::VideoDecodeAccelerator>
GLRenderingVDAClient::CreateV4L2VDA() {
  scoped_ptr<media::VideoDecodeAccelerator> decoder;
#if defined(OS_CHROMEOS) && (defined(ARCH_CPU_ARMEL) || \
    (defined(USE_OZONE) && defined(USE_V4L2_CODEC)))
  scoped_refptr<V4L2Device> device = V4L2Device::Create(V4L2Device::kDecoder);
  if (device.get()) {
    base::WeakPtr<VideoDecodeAccelerator::Client> weak_client = AsWeakPtr();
    decoder.reset(new V4L2VideoDecodeAccelerator(
        static_cast<EGLDisplay>(rendering_helper_->GetGLDisplay()),
        static_cast<EGLContext>(rendering_helper_->GetGLContextHandle()),
        weak_client,
        base::Bind(&DoNothingReturnTrue),
        device,
        base::MessageLoopProxy::current()));
  }
#endif
  return decoder.Pass();
}

scoped_ptr<media::VideoDecodeAccelerator>
GLRenderingVDAClient::CreateV4L2SliceVDA() {
  scoped_ptr<media::VideoDecodeAccelerator> decoder;
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_ARMEL) && defined(USE_LIBV4L2)
  scoped_refptr<V4L2Device> device = V4L2Device::Create(V4L2Device::kDecoder);
  if (device.get()) {
    base::WeakPtr<VideoDecodeAccelerator::Client> weak_client = AsWeakPtr();
    decoder.reset(new V4L2SliceVideoDecodeAccelerator(
        device,
        static_cast<EGLDisplay>(rendering_helper_->GetGLDisplay()),
        static_cast<EGLContext>(rendering_helper_->GetGLContextHandle()),
        weak_client,
        base::Bind(&DoNothingReturnTrue),
        base::MessageLoopProxy::current()));
  }
#endif
  return decoder.Pass();
}
scoped_ptr<media::VideoDecodeAccelerator>
GLRenderingVDAClient::CreateVaapiVDA() {
  scoped_ptr<media::VideoDecodeAccelerator> decoder;
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY)
  decoder.reset(
      new VaapiVideoDecodeAccelerator(base::Bind(&DoNothingReturnTrue)));
#endif
  return decoder.Pass();
}

void GLRenderingVDAClient::CreateAndStartDecoder() {
  CHECK(decoder_deleted());
  CHECK(!decoder_.get());

  VideoDecodeAccelerator::Client* client = this;

  scoped_ptr<media::VideoDecodeAccelerator> decoders[] = {
    CreateDXVAVDA(),
    CreateV4L2VDA(),
    CreateV4L2SliceVDA(),
    CreateVaapiVDA(),
  };

  for (size_t i = 0; i < arraysize(decoders); ++i) {
    if (!decoders[i])
      continue;
    decoder_ = decoders[i].Pass();
    weak_decoder_factory_.reset(
        new base::WeakPtrFactory<VideoDecodeAccelerator>(decoder_.get()));
    if (decoder_->Initialize(profile_, client)) {
      SetState(CS_DECODER_SET);
      FinishInitialization();
      return;
    }
  }
  // Decoders are all initialize failed.
  LOG(ERROR) << "VideoDecodeAccelerator::Initialize() failed";
  CHECK(false);
}

void GLRenderingVDAClient::ProvidePictureBuffers(
    uint32 requested_num_of_buffers,
    const gfx::Size& dimensions,
    uint32 texture_target) {
  if (decoder_deleted())
    return;
  std::vector<media::PictureBuffer> buffers;

  texture_target_ = texture_target;
  for (uint32 i = 0; i < requested_num_of_buffers; ++i) {
    uint32 texture_id;
    base::WaitableEvent done(false, false);
    rendering_helper_->CreateTexture(
        texture_target_, &texture_id, dimensions, &done);
    done.Wait();

    int32 picture_buffer_id = next_picture_buffer_id_++;
    CHECK(active_textures_
              .insert(std::make_pair(
                  picture_buffer_id,
                  new TextureRef(texture_id,
                                 base::Bind(&RenderingHelper::DeleteTexture,
                                            base::Unretained(rendering_helper_),
                                            texture_id))))
              .second);

    buffers.push_back(
        media::PictureBuffer(picture_buffer_id, dimensions, texture_id));
  }
  decoder_->AssignPictureBuffers(buffers);
}

void GLRenderingVDAClient::DismissPictureBuffer(int32 picture_buffer_id) {
  CHECK_EQ(1U, active_textures_.erase(picture_buffer_id));
}

void GLRenderingVDAClient::PictureReady(const media::Picture& picture) {
  // We shouldn't be getting pictures delivered after Reset has completed.
  CHECK_LT(state_, CS_RESET);

  if (decoder_deleted())
    return;

  base::TimeTicks now = base::TimeTicks::Now();

  frame_delivery_times_.push_back(now);

  // Save the decode time of this picture.
  std::map<int, base::TimeTicks>::iterator it =
      decode_start_time_.find(picture.bitstream_buffer_id());
  ASSERT_NE(decode_start_time_.end(), it);
  decode_time_.push_back(now - it->second);
  decode_start_time_.erase(it);

  CHECK_LE(picture.bitstream_buffer_id(), next_bitstream_buffer_id_);
  ++num_decoded_frames_;

  // Mid-stream reset applies only to the last play-through per constructor
  // comment.
  if (remaining_play_throughs_ == 1 &&
      reset_after_frame_num_ == num_decoded_frames_) {
    reset_after_frame_num_ = MID_STREAM_RESET;
    decoder_->Reset();
    // Re-start decoding from the beginning of the stream to avoid needing to
    // know how to find I-frames and so on in this test.
    encoded_data_next_pos_to_decode_ = 0;
  }

  TextureRefMap::iterator texture_it =
      active_textures_.find(picture.picture_buffer_id());
  ASSERT_NE(active_textures_.end(), texture_it);

  scoped_refptr<VideoFrameTexture> video_frame = new VideoFrameTexture(
      texture_target_, texture_it->second->texture_id(),
      base::Bind(&GLRenderingVDAClient::ReturnPicture, AsWeakPtr(),
                 picture.picture_buffer_id()));
  ASSERT_TRUE(pending_textures_.insert(*texture_it).second);

  if (render_as_thumbnails_) {
    rendering_helper_->RenderThumbnail(video_frame->texture_target(),
                                       video_frame->texture_id());
  } else if (!suppress_rendering_) {
    rendering_helper_->QueueVideoFrame(window_id_, video_frame);
  }
}

void GLRenderingVDAClient::ReturnPicture(int32 picture_buffer_id) {
  if (decoder_deleted())
    return;
  CHECK_EQ(1U, pending_textures_.erase(picture_buffer_id));

  if (pending_textures_.empty() && state_ == CS_RESETTING) {
    SetState(CS_RESET);
    DeleteDecoder();
    return;
  }

  if (num_decoded_frames_ > delay_reuse_after_frame_num_) {
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&VideoDecodeAccelerator::ReusePictureBuffer,
                   weak_decoder_factory_->GetWeakPtr(),
                   picture_buffer_id),
        kReuseDelay);
  } else {
    decoder_->ReusePictureBuffer(picture_buffer_id);
  }
}

void GLRenderingVDAClient::NotifyEndOfBitstreamBuffer(
    int32 bitstream_buffer_id) {
  // TODO(fischman): this test currently relies on this notification to make
  // forward progress during a Reset().  But the VDA::Reset() API doesn't
  // guarantee this, so stop relying on it (and remove the notifications from
  // VaapiVideoDecodeAccelerator::FinishReset()).
  ++num_done_bitstream_buffers_;
  --outstanding_decodes_;
  if (decode_calls_per_second_ == 0)
    DecodeNextFragment();
}

void GLRenderingVDAClient::NotifyFlushDone() {
  if (decoder_deleted())
    return;

  SetState(CS_FLUSHED);
  --remaining_play_throughs_;
  DCHECK_GE(remaining_play_throughs_, 0);
  if (decoder_deleted())
    return;
  decoder_->Reset();
  SetState(CS_RESETTING);
}

void GLRenderingVDAClient::NotifyResetDone() {
  if (decoder_deleted())
    return;

  if (reset_after_frame_num_ == MID_STREAM_RESET) {
    reset_after_frame_num_ = END_OF_STREAM_RESET;
    DecodeNextFragment();
    return;
  } else if (reset_after_frame_num_ == START_OF_STREAM_RESET) {
    reset_after_frame_num_ = END_OF_STREAM_RESET;
    for (int i = 0; i < num_in_flight_decodes_; ++i)
      DecodeNextFragment();
    return;
  }

  if (remaining_play_throughs_) {
    encoded_data_next_pos_to_decode_ = 0;
    FinishInitialization();
    return;
  }

  rendering_helper_->Flush(window_id_);

  if (pending_textures_.empty()) {
    SetState(CS_RESET);
    DeleteDecoder();
  }
}

void GLRenderingVDAClient::NotifyError(VideoDecodeAccelerator::Error error) {
  SetState(CS_ERROR);
}

void GLRenderingVDAClient::OutputFrameDeliveryTimes(base::File* output) {
  std::string s = base::StringPrintf("frame count: %" PRIuS "\n",
                                     frame_delivery_times_.size());
  output->WriteAtCurrentPos(s.data(), s.length());
  base::TimeTicks t0 = initialize_done_ticks_;
  for (size_t i = 0; i < frame_delivery_times_.size(); ++i) {
    s = base::StringPrintf("frame %04" PRIuS ": %" PRId64 " us\n",
                           i,
                           (frame_delivery_times_[i] - t0).InMicroseconds());
    t0 = frame_delivery_times_[i];
    output->WriteAtCurrentPos(s.data(), s.length());
  }
}

static bool LookingAtNAL(const std::string& encoded, size_t pos) {
  return encoded[pos] == 0 && encoded[pos + 1] == 0 &&
      encoded[pos + 2] == 0 && encoded[pos + 3] == 1;
}

void GLRenderingVDAClient::SetState(ClientState new_state) {
  note_->Notify(new_state);
  state_ = new_state;
  if (!remaining_play_throughs_ && new_state == delete_decoder_state_) {
    CHECK(!decoder_deleted());
    DeleteDecoder();
  }
}

void GLRenderingVDAClient::FinishInitialization() {
  SetState(CS_INITIALIZED);
  initialize_done_ticks_ = base::TimeTicks::Now();

  if (reset_after_frame_num_ == START_OF_STREAM_RESET) {
    reset_after_frame_num_ = MID_STREAM_RESET;
    decoder_->Reset();
    return;
  }

  for (int i = 0; i < num_in_flight_decodes_; ++i)
    DecodeNextFragment();
  DCHECK_EQ(outstanding_decodes_, num_in_flight_decodes_);
}

void GLRenderingVDAClient::DeleteDecoder() {
  if (decoder_deleted())
    return;
  weak_decoder_factory_.reset();
  decoder_.reset();
  STLClearObject(&encoded_data_);
  active_textures_.clear();

  // Cascade through the rest of the states to simplify test code below.
  for (int i = state_ + 1; i < CS_MAX; ++i)
    SetState(static_cast<ClientState>(i));
}

std::string GLRenderingVDAClient::GetBytesForFirstFragment(
    size_t start_pos, size_t* end_pos) {
  if (profile_ < media::H264PROFILE_MAX) {
    *end_pos = start_pos;
    while (*end_pos + 4 < encoded_data_.size()) {
      if ((encoded_data_[*end_pos + 4] & 0x1f) == 0x7) // SPS start frame
        return GetBytesForNextFragment(*end_pos, end_pos);
      GetBytesForNextNALU(*end_pos, end_pos);
      num_skipped_fragments_++;
    }
    *end_pos = start_pos;
    return std::string();
  }
  DCHECK_LE(profile_, media::VP9PROFILE_MAX);
  return GetBytesForNextFragment(start_pos, end_pos);
}

std::string GLRenderingVDAClient::GetBytesForNextFragment(
    size_t start_pos, size_t* end_pos) {
  if (profile_ < media::H264PROFILE_MAX) {
    *end_pos = start_pos;
    GetBytesForNextNALU(*end_pos, end_pos);
    if (start_pos != *end_pos) {
      num_queued_fragments_++;
    }
    return encoded_data_.substr(start_pos, *end_pos - start_pos);
  }
  DCHECK_LE(profile_, media::VP9PROFILE_MAX);
  return GetBytesForNextFrame(start_pos, end_pos);
}

void GLRenderingVDAClient::GetBytesForNextNALU(
    size_t start_pos, size_t* end_pos) {
  *end_pos = start_pos;
  if (*end_pos + 4 > encoded_data_.size())
    return;
  CHECK(LookingAtNAL(encoded_data_, start_pos));
  *end_pos += 4;
  while (*end_pos + 4 <= encoded_data_.size() &&
         !LookingAtNAL(encoded_data_, *end_pos)) {
    ++*end_pos;
  }
  if (*end_pos + 3 >= encoded_data_.size())
    *end_pos = encoded_data_.size();
}

std::string GLRenderingVDAClient::GetBytesForNextFrame(
    size_t start_pos, size_t* end_pos) {
  // Helpful description: http://wiki.multimedia.cx/index.php?title=IVF
  std::string bytes;
  if (start_pos == 0)
    start_pos = 32;  // Skip IVF header.
  *end_pos = start_pos;
  uint32 frame_size = *reinterpret_cast<uint32*>(&encoded_data_[*end_pos]);
  *end_pos += 12;  // Skip frame header.
  bytes.append(encoded_data_.substr(*end_pos, frame_size));
  *end_pos += frame_size;
  num_queued_fragments_++;
  return bytes;
}

static bool FragmentHasConfigInfo(const uint8* data, size_t size,
                                  media::VideoCodecProfile profile) {
  if (profile >= media::H264PROFILE_MIN &&
      profile <= media::H264PROFILE_MAX) {
    media::H264Parser parser;
    parser.SetStream(data, size);
    media::H264NALU nalu;
    media::H264Parser::Result result = parser.AdvanceToNextNALU(&nalu);
    if (result != media::H264Parser::kOk) {
      // Let the VDA figure out there's something wrong with the stream.
      return false;
    }

    return nalu.nal_unit_type == media::H264NALU::kSPS;
  } else if (profile >= media::VP8PROFILE_MIN &&
             profile <= media::VP9PROFILE_MAX) {
    return (size > 0 && !(data[0] & 0x01));
  }
  // Shouldn't happen at this point.
  LOG(FATAL) << "Invalid profile: " << profile;
  return false;
}

void GLRenderingVDAClient::DecodeNextFragment() {
  if (decoder_deleted())
    return;
  if (encoded_data_next_pos_to_decode_ == encoded_data_.size()) {
    if (outstanding_decodes_ == 0) {
      decoder_->Flush();
      SetState(CS_FLUSHING);
    }
    return;
  }
  size_t end_pos;
  std::string next_fragment_bytes;
  if (encoded_data_next_pos_to_decode_ == 0) {
    next_fragment_bytes = GetBytesForFirstFragment(0, &end_pos);
  } else {
    next_fragment_bytes =
        GetBytesForNextFragment(encoded_data_next_pos_to_decode_, &end_pos);
  }
  size_t next_fragment_size = next_fragment_bytes.size();

  // Call Reset() just after Decode() if the fragment contains config info.
  // This tests how the VDA behaves when it gets a reset request before it has
  // a chance to ProvidePictureBuffers().
  bool reset_here = false;
  if (reset_after_frame_num_ == RESET_AFTER_FIRST_CONFIG_INFO) {
    reset_here = FragmentHasConfigInfo(
        reinterpret_cast<const uint8*>(next_fragment_bytes.data()),
        next_fragment_size,
        profile_);
    if (reset_here)
      reset_after_frame_num_ = END_OF_STREAM_RESET;
  }

  // Populate the shared memory buffer w/ the fragment, duplicate its handle,
  // and hand it off to the decoder.
  base::SharedMemory shm;
  CHECK(shm.CreateAndMapAnonymous(next_fragment_size));
  memcpy(shm.memory(), next_fragment_bytes.data(), next_fragment_size);
  base::SharedMemoryHandle dup_handle;
  CHECK(shm.ShareToProcess(base::GetCurrentProcessHandle(), &dup_handle));
  media::BitstreamBuffer bitstream_buffer(
      next_bitstream_buffer_id_, dup_handle, next_fragment_size);
  decode_start_time_[next_bitstream_buffer_id_] = base::TimeTicks::Now();
  // Mask against 30 bits, to avoid (undefined) wraparound on signed integer.
  next_bitstream_buffer_id_ = (next_bitstream_buffer_id_ + 1) & 0x3FFFFFFF;
  decoder_->Decode(bitstream_buffer);
  ++outstanding_decodes_;
  if (!remaining_play_throughs_ &&
      -delete_decoder_state_ == next_bitstream_buffer_id_) {
    DeleteDecoder();
  }

  if (reset_here) {
    reset_after_frame_num_ = MID_STREAM_RESET;
    decoder_->Reset();
    // Restart from the beginning to re-Decode() the SPS we just sent.
    encoded_data_next_pos_to_decode_ = 0;
  } else {
    encoded_data_next_pos_to_decode_ = end_pos;
  }

  if (decode_calls_per_second_ > 0) {
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&GLRenderingVDAClient::DecodeNextFragment, AsWeakPtr()),
        base::TimeDelta::FromSeconds(1) / decode_calls_per_second_);
  }
}

double GLRenderingVDAClient::frames_per_second() {
  base::TimeDelta delta = frame_delivery_times_.back() - initialize_done_ticks_;
  return num_decoded_frames_ / delta.InSecondsF();
}

base::TimeDelta GLRenderingVDAClient::decode_time_median() {
  if (decode_time_.size() == 0)
    return base::TimeDelta();
  std::sort(decode_time_.begin(), decode_time_.end());
  int index = decode_time_.size() / 2;
  if (decode_time_.size() % 2 != 0)
    return decode_time_[index];

  return (decode_time_[index] + decode_time_[index - 1]) / 2;
}

class VideoDecodeAcceleratorTest : public ::testing::Test {
 protected:
  VideoDecodeAcceleratorTest();
  virtual void SetUp();
  virtual void TearDown();

  // Parse |data| into its constituent parts, set the various output fields
  // accordingly, and read in video stream. CHECK-fails on unexpected or
  // missing required data. Unspecified optional fields are set to -1.
  void ParseAndReadTestVideoData(base::FilePath::StringType data,
                                 std::vector<TestVideoFile*>* test_video_files);

  // Update the parameters of |test_video_files| according to
  // |num_concurrent_decoders| and |reset_point|. Ex: the expected number of
  // frames should be adjusted if decoder is reset in the middle of the stream.
  void UpdateTestVideoFileParams(
      size_t num_concurrent_decoders,
      int reset_point,
      std::vector<TestVideoFile*>* test_video_files);

  void InitializeRenderingHelper(const RenderingHelperParams& helper_params);
  void CreateAndStartDecoder(GLRenderingVDAClient* client,
                             ClientStateNotification<ClientState>* note);
  void WaitUntilDecodeFinish(ClientStateNotification<ClientState>* note);
  void WaitUntilIdle();
  void OutputLogFile(const base::FilePath::CharType* log_path,
                     const std::string& content);

  std::vector<TestVideoFile*> test_video_files_;
  RenderingHelper rendering_helper_;

 private:
  // Required for Thread to work.  Not used otherwise.
  base::ShadowingAtExitManager at_exit_manager_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecodeAcceleratorTest);
};

VideoDecodeAcceleratorTest::VideoDecodeAcceleratorTest() {
}

void VideoDecodeAcceleratorTest::SetUp() {
  ParseAndReadTestVideoData(g_test_video_data, &test_video_files_);
}

void VideoDecodeAcceleratorTest::TearDown() {
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&STLDeleteElements<std::vector<TestVideoFile*>>,
                            &test_video_files_));

  base::WaitableEvent done(false, false);
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&RenderingHelper::UnInitialize,
                            base::Unretained(&rendering_helper_), &done));
  done.Wait();

  rendering_helper_.TearDown();
}

void VideoDecodeAcceleratorTest::ParseAndReadTestVideoData(
    base::FilePath::StringType data,
    std::vector<TestVideoFile*>* test_video_files) {
  std::vector<base::FilePath::StringType> entries;
  base::SplitString(data, ';', &entries);
  CHECK_GE(entries.size(), 1U) << data;
  for (size_t index = 0; index < entries.size(); ++index) {
    std::vector<base::FilePath::StringType> fields;
    base::SplitString(entries[index], ':', &fields);
    CHECK_GE(fields.size(), 1U) << entries[index];
    CHECK_LE(fields.size(), 8U) << entries[index];
    TestVideoFile* video_file = new TestVideoFile(fields[0]);
    if (!fields[1].empty())
      CHECK(base::StringToInt(fields[1], &video_file->width));
    if (!fields[2].empty())
      CHECK(base::StringToInt(fields[2], &video_file->height));
    if (!fields[3].empty())
      CHECK(base::StringToInt(fields[3], &video_file->num_frames));
    if (!fields[4].empty())
      CHECK(base::StringToInt(fields[4], &video_file->num_fragments));
    if (!fields[5].empty())
      CHECK(base::StringToInt(fields[5], &video_file->min_fps_render));
    if (!fields[6].empty())
      CHECK(base::StringToInt(fields[6], &video_file->min_fps_no_render));
    int profile = -1;
    if (!fields[7].empty())
      CHECK(base::StringToInt(fields[7], &profile));
    video_file->profile = static_cast<media::VideoCodecProfile>(profile);

    // Read in the video data.
    base::FilePath filepath(video_file->file_name);
    CHECK(base::ReadFileToString(filepath, &video_file->data_str))
        << "test_video_file: " << filepath.MaybeAsASCII();

    test_video_files->push_back(video_file);
  }
}

void VideoDecodeAcceleratorTest::UpdateTestVideoFileParams(
    size_t num_concurrent_decoders,
    int reset_point,
    std::vector<TestVideoFile*>* test_video_files) {
  for (size_t i = 0; i < test_video_files->size(); i++) {
    TestVideoFile* video_file = (*test_video_files)[i];
    if (reset_point == MID_STREAM_RESET) {
      // Reset should not go beyond the last frame;
      // reset in the middle of the stream for short videos.
      video_file->reset_after_frame_num = kMaxResetAfterFrameNum;
      if (video_file->num_frames <= video_file->reset_after_frame_num)
        video_file->reset_after_frame_num = video_file->num_frames / 2;

      video_file->num_frames += video_file->reset_after_frame_num;
    } else {
      video_file->reset_after_frame_num = reset_point;
    }

    if (video_file->min_fps_render != -1)
      video_file->min_fps_render /= num_concurrent_decoders;
    if (video_file->min_fps_no_render != -1)
      video_file->min_fps_no_render /= num_concurrent_decoders;
  }
}

void VideoDecodeAcceleratorTest::InitializeRenderingHelper(
    const RenderingHelperParams& helper_params) {
  rendering_helper_.Setup();

  base::WaitableEvent done(false, false);
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&RenderingHelper::Initialize,
                 base::Unretained(&rendering_helper_), helper_params, &done));
  done.Wait();
}

void VideoDecodeAcceleratorTest::CreateAndStartDecoder(
    GLRenderingVDAClient* client,
    ClientStateNotification<ClientState>* note) {
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&GLRenderingVDAClient::CreateAndStartDecoder,
                            base::Unretained(client)));
  ASSERT_EQ(note->Wait(), CS_DECODER_SET);
}

void VideoDecodeAcceleratorTest::WaitUntilDecodeFinish(
    ClientStateNotification<ClientState>* note) {
  for (int i = 0; i < CS_MAX; i++) {
    if (note->Wait() == CS_DESTROYED)
      break;
  }
}

void VideoDecodeAcceleratorTest::WaitUntilIdle() {
  base::WaitableEvent done(false, false);
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&done)));
  done.Wait();
}

void VideoDecodeAcceleratorTest::OutputLogFile(
    const base::FilePath::CharType* log_path,
    const std::string& content) {
  base::File file(base::FilePath(log_path),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  file.WriteAtCurrentPos(content.data(), content.length());
}

// Test parameters:
// - Number of concurrent decoders. The value takes effect when there is only
//   one input stream; otherwise, one decoder per input stream will be
//   instantiated.
// - Number of concurrent in-flight Decode() calls per decoder.
// - Number of play-throughs.
// - reset_after_frame_num: see GLRenderingVDAClient ctor.
// - delete_decoder_phase: see GLRenderingVDAClient ctor.
// - whether to test slow rendering by delaying ReusePictureBuffer().
// - whether the video frames are rendered as thumbnails.
class VideoDecodeAcceleratorParamTest
    : public VideoDecodeAcceleratorTest,
      public ::testing::WithParamInterface<
        Tuple<int, int, int, ResetPoint, ClientState, bool, bool> > {
};

// Helper so that gtest failures emit a more readable version of the tuple than
// its byte representation.
::std::ostream& operator<<(
    ::std::ostream& os,
    const Tuple<int, int, int, ResetPoint, ClientState, bool, bool>& t) {
  return os << get<0>(t) << ", " << get<1>(t) << ", " << get<2>(t) << ", "
            << get<3>(t) << ", " << get<4>(t) << ", " << get<5>(t) << ", "
            << get<6>(t);
}

// Wait for |note| to report a state and if it's not |expected_state| then
// assert |client| has deleted its decoder.
static void AssertWaitForStateOrDeleted(
    ClientStateNotification<ClientState>* note,
    GLRenderingVDAClient* client,
    ClientState expected_state) {
  ClientState state = note->Wait();
  if (state == expected_state) return;
  ASSERT_TRUE(client->decoder_deleted())
      << "Decoder not deleted but Wait() returned " << state
      << ", instead of " << expected_state;
}

// We assert a minimal number of concurrent decoders we expect to succeed.
// Different platforms can support more concurrent decoders, so we don't assert
// failure above this.
enum { kMinSupportedNumConcurrentDecoders = 3 };

// Test the most straightforward case possible: data is decoded from a single
// chunk and rendered to the screen.
TEST_P(VideoDecodeAcceleratorParamTest, TestSimpleDecode) {
  size_t num_concurrent_decoders = get<0>(GetParam());
  const size_t num_in_flight_decodes = get<1>(GetParam());
  int num_play_throughs = get<2>(GetParam());
  const int reset_point = get<3>(GetParam());
  const int delete_decoder_state = get<4>(GetParam());
  bool test_reuse_delay = get<5>(GetParam());
  const bool render_as_thumbnails = get<6>(GetParam());

  if (test_video_files_.size() > 1)
    num_concurrent_decoders = test_video_files_.size();

  if (g_num_play_throughs > 0)
    num_play_throughs = g_num_play_throughs;

  UpdateTestVideoFileParams(
      num_concurrent_decoders, reset_point, &test_video_files_);

  // Suppress GL rendering for all tests when the "--rendering_fps" is 0.
  const bool suppress_rendering = g_rendering_fps == 0;

  std::vector<ClientStateNotification<ClientState>*>
      notes(num_concurrent_decoders, NULL);
  std::vector<GLRenderingVDAClient*> clients(num_concurrent_decoders, NULL);

  RenderingHelperParams helper_params;
  helper_params.rendering_fps = g_rendering_fps;
  helper_params.warm_up_iterations = g_rendering_warm_up;
  helper_params.render_as_thumbnails = render_as_thumbnails;
  if (render_as_thumbnails) {
    // Only one decoder is supported with thumbnail rendering
    CHECK_EQ(num_concurrent_decoders, 1U);
    helper_params.thumbnails_page_size = kThumbnailsPageSize;
    helper_params.thumbnail_size = kThumbnailSize;
  }

  // First kick off all the decoders.
  for (size_t index = 0; index < num_concurrent_decoders; ++index) {
    TestVideoFile* video_file =
        test_video_files_[index % test_video_files_.size()];
    ClientStateNotification<ClientState>* note =
        new ClientStateNotification<ClientState>();
    notes[index] = note;

    int delay_after_frame_num = std::numeric_limits<int>::max();
    if (test_reuse_delay &&
        kMaxFramesToDelayReuse * 2 < video_file->num_frames) {
      delay_after_frame_num = video_file->num_frames - kMaxFramesToDelayReuse;
    }

    GLRenderingVDAClient* client =
        new GLRenderingVDAClient(index,
                                 &rendering_helper_,
                                 note,
                                 video_file->data_str,
                                 num_in_flight_decodes,
                                 num_play_throughs,
                                 video_file->reset_after_frame_num,
                                 delete_decoder_state,
                                 video_file->width,
                                 video_file->height,
                                 video_file->profile,
                                 suppress_rendering,
                                 delay_after_frame_num,
                                 0,
                                 render_as_thumbnails);

    clients[index] = client;
    helper_params.window_sizes.push_back(
        render_as_thumbnails
            ? kThumbnailsPageSize
            : gfx::Size(video_file->width, video_file->height));
  }

  InitializeRenderingHelper(helper_params);

  for (size_t index = 0; index < num_concurrent_decoders; ++index) {
    CreateAndStartDecoder(clients[index], notes[index]);
  }

  // Then wait for all the decodes to finish.
  // Only check performance & correctness later if we play through only once.
  bool skip_performance_and_correctness_checks = num_play_throughs > 1;
  for (size_t i = 0; i < num_concurrent_decoders; ++i) {
    ClientStateNotification<ClientState>* note = notes[i];
    ClientState state = note->Wait();
    if (state != CS_INITIALIZED) {
      skip_performance_and_correctness_checks = true;
      // We expect initialization to fail only when more than the supported
      // number of decoders is instantiated.  Assert here that something else
      // didn't trigger failure.
      ASSERT_GT(num_concurrent_decoders,
                static_cast<size_t>(kMinSupportedNumConcurrentDecoders));
      continue;
    }
    ASSERT_EQ(state, CS_INITIALIZED);
    for (int n = 0; n < num_play_throughs; ++n) {
      // For play-throughs other than the first, we expect initialization to
      // succeed unconditionally.
      if (n > 0) {
        ASSERT_NO_FATAL_FAILURE(
            AssertWaitForStateOrDeleted(note, clients[i], CS_INITIALIZED));
      }
      // InitializeDone kicks off decoding inside the client, so we just need to
      // wait for Flush.
      ASSERT_NO_FATAL_FAILURE(
          AssertWaitForStateOrDeleted(note, clients[i], CS_FLUSHING));
      ASSERT_NO_FATAL_FAILURE(
          AssertWaitForStateOrDeleted(note, clients[i], CS_FLUSHED));
      // FlushDone requests Reset().
      ASSERT_NO_FATAL_FAILURE(
          AssertWaitForStateOrDeleted(note, clients[i], CS_RESETTING));
    }
    ASSERT_NO_FATAL_FAILURE(
        AssertWaitForStateOrDeleted(note, clients[i], CS_RESET));
    // ResetDone requests Destroy().
    ASSERT_NO_FATAL_FAILURE(
        AssertWaitForStateOrDeleted(note, clients[i], CS_DESTROYED));
  }
  // Finally assert that decoding went as expected.
  for (size_t i = 0; i < num_concurrent_decoders &&
           !skip_performance_and_correctness_checks; ++i) {
    // We can only make performance/correctness assertions if the decoder was
    // allowed to finish.
    if (delete_decoder_state < CS_FLUSHED)
      continue;
    GLRenderingVDAClient* client = clients[i];
    TestVideoFile* video_file = test_video_files_[i % test_video_files_.size()];
    if (video_file->num_frames > 0) {
      // Expect the decoded frames may be more than the video frames as frames
      // could still be returned until resetting done.
      if (video_file->reset_after_frame_num > 0)
        EXPECT_GE(client->num_decoded_frames(), video_file->num_frames);
      else
        EXPECT_EQ(client->num_decoded_frames(), video_file->num_frames);
    }
    if (reset_point == END_OF_STREAM_RESET) {
      EXPECT_EQ(video_file->num_fragments, client->num_skipped_fragments() +
                client->num_queued_fragments());
      EXPECT_EQ(client->num_done_bitstream_buffers(),
                client->num_queued_fragments());
    }
    LOG(INFO) << "Decoder " << i << " fps: " << client->frames_per_second();
    if (!render_as_thumbnails) {
      int min_fps = suppress_rendering ?
          video_file->min_fps_no_render : video_file->min_fps_render;
      if (min_fps > 0 && !test_reuse_delay)
        EXPECT_GT(client->frames_per_second(), min_fps);
    }
  }

  if (render_as_thumbnails) {
    std::vector<unsigned char> rgb;
    bool alpha_solid;
    base::WaitableEvent done(false, false);
    g_env->GetRenderingTaskRunner()->PostTask(
        FROM_HERE, base::Bind(&RenderingHelper::GetThumbnailsAsRGB,
                              base::Unretained(&rendering_helper_), &rgb,
                              &alpha_solid, &done));
    done.Wait();

    std::vector<std::string> golden_md5s;
    std::string md5_string = base::MD5String(
        base::StringPiece(reinterpret_cast<char*>(&rgb[0]), rgb.size()));
    ReadGoldenThumbnailMD5s(test_video_files_[0], &golden_md5s);
    std::vector<std::string>::iterator match =
        find(golden_md5s.begin(), golden_md5s.end(), md5_string);
    if (match == golden_md5s.end()) {
      // Convert raw RGB into PNG for export.
      std::vector<unsigned char> png;
      gfx::PNGCodec::Encode(&rgb[0],
                            gfx::PNGCodec::FORMAT_RGB,
                            kThumbnailsPageSize,
                            kThumbnailsPageSize.width() * 3,
                            true,
                            std::vector<gfx::PNGCodec::Comment>(),
                            &png);

      LOG(ERROR) << "Unknown thumbnails MD5: " << md5_string;

      base::FilePath filepath(test_video_files_[0]->file_name);
      filepath = filepath.AddExtension(FILE_PATH_LITERAL(".bad_thumbnails"));
      filepath = filepath.AddExtension(FILE_PATH_LITERAL(".png"));
      int num_bytes = base::WriteFile(filepath,
                                           reinterpret_cast<char*>(&png[0]),
                                           png.size());
      ASSERT_EQ(num_bytes, static_cast<int>(png.size()));
    }
    ASSERT_NE(match, golden_md5s.end());
    EXPECT_EQ(alpha_solid, true) << "RGBA frame had incorrect alpha";
  }

  // Output the frame delivery time to file
  // We can only make performance/correctness assertions if the decoder was
  // allowed to finish.
  if (g_output_log != NULL && delete_decoder_state >= CS_FLUSHED) {
    base::File output_file(
        base::FilePath(g_output_log),
        base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    for (size_t i = 0; i < num_concurrent_decoders; ++i) {
      clients[i]->OutputFrameDeliveryTimes(&output_file);
    }
  }

  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&STLDeleteElements<std::vector<GLRenderingVDAClient*>>,
                 &clients));
  g_env->GetRenderingTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&STLDeleteElements<
                     std::vector<ClientStateNotification<ClientState>*>>,
                 &notes));
  WaitUntilIdle();
};

// Test that replay after EOS works fine.
INSTANTIATE_TEST_CASE_P(
    ReplayAfterEOS, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 4, END_OF_STREAM_RESET, CS_RESET, false, false)));

// Test that Reset() before the first Decode() works fine.
INSTANTIATE_TEST_CASE_P(
    ResetBeforeDecode, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, START_OF_STREAM_RESET, CS_RESET, false, false)));

// Test Reset() immediately after Decode() containing config info.
INSTANTIATE_TEST_CASE_P(
    ResetAfterFirstConfigInfo, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(
            1, 1, 1, RESET_AFTER_FIRST_CONFIG_INFO, CS_RESET, false, false)));

// Test that Reset() mid-stream works fine and doesn't affect decoding even when
// Decode() calls are made during the reset.
INSTANTIATE_TEST_CASE_P(
    MidStreamReset, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, MID_STREAM_RESET, CS_RESET, false, false)));

INSTANTIATE_TEST_CASE_P(
    SlowRendering, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_RESET, true, false)));

// Test that Destroy() mid-stream works fine (primarily this is testing that no
// crashes occur).
INSTANTIATE_TEST_CASE_P(
    TearDownTiming, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_DECODER_SET, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_INITIALIZED, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_FLUSHING, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_FLUSHED, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_RESETTING, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_RESET, false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET,
                  static_cast<ClientState>(-1), false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET,
                  static_cast<ClientState>(-10), false, false),
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET,
                  static_cast<ClientState>(-100), false, false)));

// Test that decoding various variation works with multiple in-flight decodes.
INSTANTIATE_TEST_CASE_P(
    DecodeVariations, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_RESET, false, false),
        MakeTuple(1, 10, 1, END_OF_STREAM_RESET, CS_RESET, false, false),
        // Tests queuing.
        MakeTuple(1, 15, 1, END_OF_STREAM_RESET, CS_RESET, false, false)));

// Find out how many concurrent decoders can go before we exhaust system
// resources.
INSTANTIATE_TEST_CASE_P(
    ResourceExhaustion, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        // +0 hack below to promote enum to int.
        MakeTuple(kMinSupportedNumConcurrentDecoders + 0, 1, 1,
                  END_OF_STREAM_RESET, CS_RESET, false, false),
        MakeTuple(kMinSupportedNumConcurrentDecoders + 1, 1, 1,
                  END_OF_STREAM_RESET, CS_RESET, false, false)));

// Thumbnailing test
INSTANTIATE_TEST_CASE_P(
    Thumbnail, VideoDecodeAcceleratorParamTest,
    ::testing::Values(
        MakeTuple(1, 1, 1, END_OF_STREAM_RESET, CS_RESET, false, true)));

// Measure the median of the decode time when VDA::Decode is called 30 times per
// second.
TEST_F(VideoDecodeAcceleratorTest, TestDecodeTimeMedian) {
  RenderingHelperParams helper_params;

  // Disable rendering by setting the rendering_fps = 0.
  helper_params.rendering_fps = 0;
  helper_params.warm_up_iterations = 0;
  helper_params.render_as_thumbnails = false;

  ClientStateNotification<ClientState>* note =
      new ClientStateNotification<ClientState>();
  GLRenderingVDAClient* client =
      new GLRenderingVDAClient(0,
                               &rendering_helper_,
                               note,
                               test_video_files_[0]->data_str,
                               1,
                               1,
                               test_video_files_[0]->reset_after_frame_num,
                               CS_RESET,
                               test_video_files_[0]->width,
                               test_video_files_[0]->height,
                               test_video_files_[0]->profile,
                               true,
                               std::numeric_limits<int>::max(),
                               kWebRtcDecodeCallsPerSecond,
                               false /* render_as_thumbnail */);
  helper_params.window_sizes.push_back(
      gfx::Size(test_video_files_[0]->width, test_video_files_[0]->height));
  InitializeRenderingHelper(helper_params);
  CreateAndStartDecoder(client, note);
  WaitUntilDecodeFinish(note);

  base::TimeDelta decode_time_median = client->decode_time_median();
  std::string output_string =
      base::StringPrintf("Decode time median: %" PRId64 " us",
                         decode_time_median.InMicroseconds());
  LOG(INFO) << output_string;

  if (g_output_log != NULL)
    OutputLogFile(g_output_log, output_string);

  g_env->GetRenderingTaskRunner()->DeleteSoon(FROM_HERE, client);
  g_env->GetRenderingTaskRunner()->DeleteSoon(FROM_HERE, note);
  WaitUntilIdle();
};

// TODO(fischman, vrk): add more tests!  In particular:
// - Test life-cycle: Seek/Stop/Pause/Play for a single decoder.
// - Test alternate configurations
// - Test failure conditions.
// - Test frame size changes mid-stream

}  // namespace
}  // namespace content

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);  // Removes gtest-specific args.
  base::CommandLine::Init(argc, argv);

  // Needed to enable DVLOG through --vmodule.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  CHECK(logging::InitLogging(settings));

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  DCHECK(cmd_line);

  base::CommandLine::SwitchMap switches = cmd_line->GetSwitches();
  for (base::CommandLine::SwitchMap::const_iterator it = switches.begin();
       it != switches.end(); ++it) {
    if (it->first == "test_video_data") {
      content::g_test_video_data = it->second.c_str();
      continue;
    }
    // The output log for VDA performance test.
    if (it->first == "output_log") {
      content::g_output_log = it->second.c_str();
      continue;
    }
    if (it->first == "rendering_fps") {
      // On Windows, CommandLine::StringType is wstring. We need to convert
      // it to std::string first
      std::string input(it->second.begin(), it->second.end());
      CHECK(base::StringToDouble(input, &content::g_rendering_fps));
      continue;
    }
    if (it->first == "rendering_warm_up") {
      std::string input(it->second.begin(), it->second.end());
      CHECK(base::StringToInt(input, &content::g_rendering_warm_up));
      continue;
    }
    // TODO(owenlin): Remove this flag once it is not used in autotest.
    if (it->first == "disable_rendering") {
      content::g_rendering_fps = 0;
      continue;
    }

    if (it->first == "num_play_throughs") {
      std::string input(it->second.begin(), it->second.end());
      CHECK(base::StringToInt(input, &content::g_num_play_throughs));
      continue;
    }
    if (it->first == "v" || it->first == "vmodule")
      continue;
    if (it->first == "ozone-platform" || it->first == "ozone-use-surfaceless")
      continue;
    LOG(FATAL) << "Unexpected switch: " << it->first << ":" << it->second;
  }

  base::ShadowingAtExitManager at_exit_manager;
#if defined(OS_WIN) || defined(USE_OZONE)
  // For windows the decoding thread initializes the media foundation decoder
  // which uses COM. We need the thread to be a UI thread.
  // On Ozone, the backend initializes the event system using a UI
  // thread.
  base::MessageLoopForUI main_loop;
#else
  base::MessageLoop main_loop;
#endif  // OS_WIN || USE_OZONE

#if defined(USE_OZONE)
  ui::OzonePlatform::InitializeForUI();
#endif

  content::g_env =
      reinterpret_cast<content::VideoDecodeAcceleratorTestEnvironment*>(
          testing::AddGlobalTestEnvironment(
              new content::VideoDecodeAcceleratorTestEnvironment()));

  return RUN_ALL_TESTS();
}
