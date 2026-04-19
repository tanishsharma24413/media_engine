#include "media/engine.hpp"

#include <string>
#include <utility>

namespace media {

Engine::Engine() : worker_{[this] { thread_main(); }} {}

Engine::~Engine() { post_shutdown(); }

void Engine::set_on_state(OnState cb) {
  std::scoped_lock lock(cb_mutex_);
  on_state_ = std::move(cb);
}

void Engine::set_on_error(OnError cb) {
  std::scoped_lock lock(cb_mutex_);
  on_error_ = std::move(cb);
}

void Engine::set_pipeline(std::shared_ptr<MediaPipeline> pipeline) {
  if (!pipeline) {
    emit_error("set_pipeline(null)");
    return;
  }
  pipeline_ = std::move(pipeline);
}

void Engine::post_open(MediaUri uri) {
  std::unique_lock lock(ch_.m);
  ch_.cv.wait(lock, [this] { return !ch_.has; });
  ch_.cmd = Command{CommandType::Open, std::move(uri)};
  ch_.has = true;
  ch_.cv.notify_one();
}

void Engine::post_play() {
  std::unique_lock lock(ch_.m);
  ch_.cv.wait(lock, [this] { return !ch_.has; });
  ch_.cmd = Command{CommandType::Play, {}};
  ch_.has = true;
  ch_.cv.notify_one();
}

void Engine::post_pause() {
  std::unique_lock lock(ch_.m);
  ch_.cv.wait(lock, [this] { return !ch_.has; });
  ch_.cmd = Command{CommandType::Pause, {}};
  ch_.has = true;
  ch_.cv.notify_one();
}

void Engine::post_stop() {
  std::unique_lock lock(ch_.m);
  ch_.cv.wait(lock, [this] { return !ch_.has; });
  ch_.cmd = Command{CommandType::Stop, {}};
  ch_.has = true;
  ch_.cv.notify_one();
}

void Engine::post_shutdown() {
  shutdown_requested_.store(true);
  {
    std::unique_lock lock(ch_.m);
    ch_.cv.wait(lock, [this] { return !ch_.has; });
    ch_.cmd = Command{CommandType::Shutdown, {}};
    ch_.has = true;
  }
  ch_.cv.notify_one();
  if (worker_.joinable()) {
    worker_.join();
  }
}

PlayerState Engine::state() const { return state_.load(); }

void Engine::thread_main() {
  while (true) {
    Command cmd{};
    {
      std::unique_lock lock(ch_.m);
      ch_.cv.wait(lock, [this] { return ch_.has || shutdown_requested_.load(); });
      if (!ch_.has) {
        if (shutdown_requested_.load()) {
          return;
        }
        continue;
      }
      cmd = ch_.cmd;
    }

    switch (cmd.type) {
    case CommandType::Open: {
      emit_state(PlayerState::Opening);
      pipeline_->attach_uri(std::move(cmd.uri));
      emit_state(PlayerState::Paused);
      break;
    }
    case CommandType::Play:
      emit_state(PlayerState::Playing);
      break;
    case CommandType::Pause:
      emit_state(PlayerState::Paused);
      break;
    case CommandType::Stop:
      pipeline_->clear();
      emit_state(PlayerState::Stopped);
      break;
    case CommandType::Shutdown:
      pipeline_->clear();
      emit_state(PlayerState::Idle);
      {
        std::scoped_lock lock(ch_.m);
        ch_.has = false;
      }
      ch_.cv.notify_all();
      return;
    }

    {
      std::scoped_lock lock(ch_.m);
      ch_.has = false;
    }
    ch_.cv.notify_all();
  }
}

void Engine::emit_state(PlayerState s) {
  state_.store(s);
  OnState cb{};
  {
    std::scoped_lock lock(cb_mutex_);
    cb = on_state_;
  }
  if (cb) {
    cb(s);
  }
}

void Engine::emit_error(std::string message) {
  emit_state(PlayerState::Error);
  OnError cb{};
  {
    std::scoped_lock lock(cb_mutex_);
    cb = on_error_;
  }
  if (cb) {
    cb(EngineError{std::move(message)});
  }
}

} // namespace media
