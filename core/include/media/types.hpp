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

struct MediaPacket {
  void* opaque{nullptr}; // Backend-specific packet (e.g. AVPacket)
  bool is_video{false};
  bool is_audio{false};
  double pts_seconds{0.0};
};

struct MediaFrame {
  void* opaque{nullptr}; // Backend-specific frame (e.g. AVFrame)
  double pts_seconds{0.0};
  int width{0};
  int height{0};
};

} // namespace media
