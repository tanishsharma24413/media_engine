#include "media/pipeline.hpp"

namespace media {

MediaPipeline::MediaPipeline() = default;
MediaPipeline::~MediaPipeline() = default;

void MediaPipeline::attach_uri(MediaUri uri) {
  std::scoped_lock lock(mutex_);
  uri_ = std::move(uri);
}

void MediaPipeline::clear() {
  std::scoped_lock lock(mutex_);
  uri_ = {};
  source_.reset();
  audio_.reset();
  video_.reset();
}

void MediaPipeline::set_packet_source(std::shared_ptr<IPacketSource> source) {
  std::scoped_lock lock(mutex_);
  source_ = std::move(source);
}

void MediaPipeline::set_video_decoder(std::shared_ptr<IDecoder> decoder) {
  std::scoped_lock lock(mutex_);
  video_decoder_ = std::move(decoder);
}

void MediaPipeline::set_audio_output(std::shared_ptr<IAudioOutput> audio) {
  std::scoped_lock lock(mutex_);
  audio_ = std::move(audio);
}

void MediaPipeline::set_video_output(std::shared_ptr<IVideoOutput> video) {
  std::scoped_lock lock(mutex_);
  video_ = std::move(video);
}

MediaUri MediaPipeline::uri() const {
  std::scoped_lock lock(mutex_);
  return uri_;
}

} // namespace media
