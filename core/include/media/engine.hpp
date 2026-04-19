#pragma once

#include "media/pipeline.hpp"
#include "media/types.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace media {

class Engine {
public:
  using OnState = std::function<void(PlayerState)>;
  using OnError = std::function<void(EngineError const&)>;

  Engine();
  ~Engine();

  void set_on_state(OnState cb);
  void set_on_error(OnError cb);

  void set_pipeline(std::shared_ptr<MediaPipeline> pipeline);

  void post_open(MediaUri uri);
  void post_play();
  void post_pause();
  void post_stop();
  void post_shutdown();

  PlayerState state() const;

private:
  void thread_main();
  void emit_state(PlayerState s);
  void emit_error(std::string message);

  std::shared_ptr<MediaPipeline> pipeline_{std::make_shared<MediaPipeline>()};

  std::mutex cb_mutex_{};
  OnState on_state_{};
  OnError on_error_{};

  std::atomic<PlayerState> state_{PlayerState::Idle};
  std::atomic<bool> shutdown_requested_{false};

  enum class CommandType : std::uint8_t { Open, Play, Pause, Stop, Shutdown };

  struct Command {
    CommandType type{CommandType::Shutdown};
    MediaUri uri{};
  };

  struct Channel {
    std::mutex m;
    std::condition_variable cv;
    bool has{false};
    Command cmd{};
  };

  Channel ch_{};
  std::thread worker_{};
};

} // namespace media
