#pragma once

#include <memory>

namespace media {
struct IPacketSource;
struct IDecoder;
}

namespace media::ffmpeg {

std::shared_ptr<IPacketSource> create_ffmpeg_demuxer();
std::shared_ptr<IDecoder> create_ffmpeg_decoder();

void bootstrap_ffmpeg_backend();

} // namespace media::ffmpeg
