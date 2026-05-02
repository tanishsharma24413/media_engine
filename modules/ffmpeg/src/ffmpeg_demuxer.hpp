#pragma once

#include "media/pipeline.hpp"
#include <string>

struct AVFormatContext;

namespace media::ffmpeg {

class FFmpegDemuxer : public IPacketSource {
public:
  FFmpegDemuxer();
  ~FFmpegDemuxer() override;

  bool open(MediaUri const& uri) override;
  bool read_packet(MediaPacket& pkt) override;
  void free_packet(MediaPacket& pkt) override;

  StreamInfo video_info() const override;
  StreamInfo audio_info() const override;

private:
  AVFormatContext* format_ctx_{nullptr};
  int video_stream_index_{-1};
  int audio_stream_index_{-1};
};

} // namespace media::ffmpeg
