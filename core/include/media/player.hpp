#pragma once

#include "media/pipeline.hpp"
#include "media/types.hpp"

#include <functional>
#include <memory>
#include <string>

namespace media {

class Engine;
class MediaPipeline;

// Small stable surface for UI/apps. Implementation details stay out of headers.
class Player {
public:
  using OnState = std::function<void(PlayerState)>;
  using OnError = std::function<void(std::string const&)>;

  Player();
  ~Player();

  Player(Player const&) = delete;
  Player& operator=(Player const&) = delete;
  Player(Player&&) noexcept;
  Player& operator=(Player&&) noexcept;

  void set_on_state(OnState cb);
  void set_on_error(OnError cb);

  // Thread-safe: posts to the engine thread.
  void open(MediaUri uri);
  void play();
  void pause();
  void stop();

  PlayerState state() const;

  // Playback info (safe to call from any thread).
  double position_seconds() const;
  double duration_seconds() const;
  void   set_volume(float gain);  // gain in [0.0, 1.0]

  // Advanced: replace default pipeline (e.g. inject test doubles). Not required for apps.
  void set_pipeline(std::shared_ptr<MediaPipeline> pipeline);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace media
