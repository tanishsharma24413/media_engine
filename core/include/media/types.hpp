#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace media {

using ClockDuration = std::chrono::duration<double, std::ratio<1>>;

struct MediaUri {
  std::string value;
};

enum class PlayerState : std::uint8_t {
  Idle,
  Opening,
  Playing,
  Paused,
  Stopped,
  Error,
};

struct EngineError {
  std::string message;
};

} // namespace media
