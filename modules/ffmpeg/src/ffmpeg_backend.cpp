#include "media/modules/ffmpeg/ffmpeg_backend.hpp"
#include "ffmpeg_demuxer.hpp"
#include "ffmpeg_decoder.hpp"
#include "media/pipeline.hpp"

#include <memory>

namespace media::ffmpeg {

std::shared_ptr<IPacketSource> create_ffmpeg_demuxer() {
  return std::make_shared<FFmpegDemuxer>();
}

std::shared_ptr<IDecoder> create_ffmpeg_decoder() {
  return std::make_shared<FFmpegDecoder>();
}

void bootstrap_ffmpeg_backend() {
  // Not used directly if Engine explicitly instantiates them.
}

} // namespace media::ffmpeg
