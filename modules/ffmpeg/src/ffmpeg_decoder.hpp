#pragma once

#include "media/pipeline.hpp"

struct AVCodecContext;
struct SwsContext;
struct SwrContext;

namespace media::ffmpeg {

class FFmpegDecoder : public IDecoder {
public:
  FFmpegDecoder();
  ~FFmpegDecoder() override;

  bool open(StreamInfo const& info) override;
  bool send_packet(MediaPacket const& pkt) override;
  bool receive_frame(MediaFrame& frame) override;
  void free_frame(MediaFrame& frame) override;

private:
  AVCodecContext* codec_ctx_{nullptr};
  SwsContext* sws_ctx_{nullptr};
  SwrContext* swr_ctx_{nullptr};
  double time_base_{0.0};
  bool is_audio_{false};
};

} // namespace media::ffmpeg
