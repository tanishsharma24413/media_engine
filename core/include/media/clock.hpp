#pragma once

#include "media/types.hpp"

#include <atomic>
#include <chrono>

namespace media {

// Owns playback timebase policy (v1: monotonic "media clock" placeholder).
class PlaybackClock {
public:
  PlaybackClock();

  void reset(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
  void set_paused(bool paused, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

  // Elapsed media time while not paused (stub until real A/V sync exists).
  ClockDuration elapsed(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const;

private:
  std::chrono::steady_clock::time_point origin_{};
  std::chrono::steady_clock::time_point pause_began_{};
  ClockDuration accumulated_{};
  bool paused_{true};
};

} // namespace media
