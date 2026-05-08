#include "media/player.hpp"

#include "media/engine.hpp"

namespace media {

struct Player::Impl {
  Engine engine{};
};

Player::Player() : impl_(std::make_unique<Impl>()) {}

Player::~Player() = default;

Player::Player(Player&&) noexcept = default;
Player& Player::operator=(Player&&) noexcept = default;

void Player::set_on_state(OnState cb) { impl_->engine.set_on_state(std::move(cb)); }

void Player::set_on_error(OnError cb) {
  impl_->engine.set_on_error([cb](EngineError const& e) {
    if (cb) {
      cb(e.message);
    }
  });
}

void Player::open(MediaUri uri) { impl_->engine.post_open(std::move(uri)); }

void Player::play() { impl_->engine.post_play(); }

void Player::pause() { impl_->engine.post_pause(); }

void Player::stop() { impl_->engine.post_stop(); }

PlayerState Player::state() const { return impl_->engine.state(); }

double Player::position_seconds() const { return impl_->engine.position_seconds(); }
double Player::duration_seconds() const { return impl_->engine.duration_seconds(); }
void   Player::set_volume(float gain)   { impl_->engine.set_volume(gain); }

void Player::set_pipeline(std::shared_ptr<MediaPipeline> pipeline) {
  impl_->engine.set_pipeline(std::move(pipeline));
}

} // namespace media
