#pragma once

#include "media/types.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace media {

struct IPacketSource {
  virtual ~IPacketSource() = default;
};

struct IAudioOutput {
  virtual ~IAudioOutput() = default;
};

struct IVideoOutput {
  virtual ~IVideoOutput() = default;
};

// Owns graph wiring and flush/seek barriers (v1: holds collaborators only).
class MediaPipeline {
public:
  MediaPipeline();
  ~MediaPipeline();

  void attach_uri(MediaUri uri);
  void clear();

  void set_packet_source(std::shared_ptr<IPacketSource> source);
  void set_audio_output(std::shared_ptr<IAudioOutput> audio);
  void set_video_output(std::shared_ptr<IVideoOutput> video);

  MediaUri uri() const;

private:
  mutable std::mutex mutex_{};
  MediaUri uri_{};
  std::shared_ptr<IPacketSource> source_{};
  std::shared_ptr<IAudioOutput> audio_{};
  std::shared_ptr<IVideoOutput> video_{};
};

} // namespace media
