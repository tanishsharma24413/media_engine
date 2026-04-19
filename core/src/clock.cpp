#include "media/clock.hpp"

namespace media {

PlaybackClock::PlaybackClock() { reset(); }

void PlaybackClock::reset(std::chrono::steady_clock::time_point now) {
  origin_ = now;
  pause_began_ = now;
  accumulated_ = ClockDuration{0};
  paused_ = true;
}

void PlaybackClock::set_paused(bool paused, std::chrono::steady_clock::time_point now) {
  if (paused_ == paused) {
    return;
  }

  if (paused) {
    accumulated_ += ClockDuration{now - origin_};
    pause_began_ = now;
    paused_ = true;
    return;
  }

  origin_ = now;
  paused_ = false;
}

ClockDuration PlaybackClock::elapsed(std::chrono::steady_clock::time_point now) const {
  if (paused_) {
    return accumulated_;
  }
  return accumulated_ + ClockDuration{now - origin_};
}

} // namespace media
