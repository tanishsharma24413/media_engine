#include "media/engine.hpp"
#include "media/clock.hpp"

#include <string>
#include <utility>

namespace media {

Engine::Engine() : state_worker_{[this] { thread_main(); }} {}

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
  if (state_worker_.joinable()) {
    state_worker_.join();
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
      
      if (pipeline_->source()) {
          pipeline_->source()->open(pipeline_->uri());
          if (pipeline_->video_decoder()) {
              pipeline_->video_decoder()->open(pipeline_->source()->video_info());
          }
      }

      emit_state(PlayerState::Paused);
      break;
    }
    case CommandType::Play:
      if (state_.load() == PlayerState::Stopped || state_.load() == PlayerState::Idle) {
         // Should not happen if open was called, but safety first
      } else if (state_.load() != PlayerState::Playing) {
         start_playback_threads();
      }
      emit_state(PlayerState::Playing);
      break;
    case CommandType::Pause:
      emit_state(PlayerState::Paused);
      break;
    case CommandType::Stop:
      stop_playback_threads();
      pipeline_->clear();
      emit_state(PlayerState::Stopped);
      break;
    case CommandType::Shutdown:
      stop_playback_threads();
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

void Engine::start_playback_threads() {
  stop_playback_threads();
  packet_queue_.reset();
  frame_queue_.reset();
  clock_.reset();
  
  if (pipeline_->source()) {
    input_worker_ = std::thread([this] { input_thread_main(); });
  }
  if (pipeline_->video_decoder()) {
    decoder_worker_ = std::thread([this] { decoder_thread_main(); });
  }
  if (pipeline_->video_output()) {
    vout_worker_ = std::thread([this] { vout_thread_main(); });
  }
}

void Engine::stop_playback_threads() {
  packet_queue_.flush();
  frame_queue_.flush();

  if (input_worker_.joinable()) input_worker_.join();
  if (decoder_worker_.joinable()) decoder_worker_.join();
  if (vout_worker_.joinable()) vout_worker_.join();
}

void Engine::input_thread_main() {
  auto source = pipeline_->source();
  if (!source) return;

  while (state_.load() != PlayerState::Stopped && state_.load() != PlayerState::Idle) {
    if (state_.load() == PlayerState::Paused) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }

    // Very simple VLC-like demux loop. Limit queue size to avoid OOM.
    if (packet_queue_.size() > 100) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    MediaPacket pkt{};
    if (source->read_packet(pkt)) {
      packet_queue_.push(std::move(pkt));
    } else {
      // EOF or error
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void Engine::decoder_thread_main() {
  auto decoder = pipeline_->video_decoder();
  if (!decoder) return;

  while (state_.load() != PlayerState::Stopped && state_.load() != PlayerState::Idle) {
    auto opt_pkt = packet_queue_.pop();
    if (!opt_pkt) break; // flushed

    MediaPacket pkt = std::move(*opt_pkt);
    if (pkt.is_video) {
      if (decoder->send_packet(pkt)) {
        MediaFrame frame{};
        while (decoder->receive_frame(frame)) {
          frame_queue_.push(std::move(frame));
          frame = {};
        }
      }
    }
    
    // Packet is consumed, we should free its opaque data (if backend didn't take ownership).
    // VLC handles this inside the decoder or drops it. We'll rely on pipeline->source to free it.
    if (pipeline_->source()) {
       pipeline_->source()->free_packet(pkt);
    }
  }
}

void Engine::vout_thread_main() {
  auto vout = pipeline_->video_output();
  if (!vout) return;

  while (state_.load() != PlayerState::Stopped && state_.load() != PlayerState::Idle) {
    if (state_.load() == PlayerState::Paused) {
      clock_.set_paused(true);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }
    clock_.set_paused(false);

    auto opt_frame = frame_queue_.pop();
    if (!opt_frame) break;

    MediaFrame frame = std::move(*opt_frame);
    
    // VLC logic: check PTS against master clock. 
    double pts = frame.pts_seconds;
    double elapsed = clock_.elapsed().count();
    
    if (pts > elapsed) {
        double diff = pts - elapsed;
        // Cap sleep to avoid massive hangs on first frame or PTS jumps
        if (diff > 0.0 && diff < 5.0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(diff));
        } else if (diff >= 5.0) {
            // If jump is huge, reset clock to PTS
            clock_.reset();
        }
    }

    vout->render_frame(frame);

    if (pipeline_->video_decoder()) {
       pipeline_->video_decoder()->free_frame(frame);
    }
  }
}

} // namespace media
