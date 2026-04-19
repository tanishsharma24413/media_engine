#pragma once

namespace media::ffmpeg {

// Future: construct demux/decode adapters behind core interfaces.
void bootstrap_ffmpeg_backend();

} // namespace media::ffmpeg
