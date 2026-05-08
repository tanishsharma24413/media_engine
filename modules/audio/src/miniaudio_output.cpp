#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "miniaudio_output.hpp"

#include <cstring>

namespace media::audio {

MiniaudioOutput::MiniaudioOutput() = default;

MiniaudioOutput::~MiniaudioOutput() { shutdown(); }

bool MiniaudioOutput::open(int sample_rate, int channels) {
  shutdown();

  rb_ = new ma_pcm_rb;
  // Initialize ring buffer for 1 second of audio
  if (ma_pcm_rb_init(ma_format_f32, channels, sample_rate, nullptr, nullptr, rb_) != MA_SUCCESS) {
      delete rb_;
      rb_ = nullptr;
      return false;
  }

  device_ = new ma_device;
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format   = ma_format_f32;
  config.playback.channels = channels;
  config.sampleRate        = sample_rate;
  config.dataCallback      = data_callback;
  config.pUserData         = this;

  if (ma_device_init(nullptr, &config, device_) != MA_SUCCESS) {
    shutdown();
    return false;
  }

  if (ma_device_start(device_) != MA_SUCCESS) {
    shutdown();
    return false;
  }

  return true;
}

void MiniaudioOutput::shutdown() {
  if (device_) {
    ma_device_uninit(device_);
    delete device_;
    device_ = nullptr;
  }
  if (rb_) {
    ma_pcm_rb_uninit(rb_);
    delete rb_;
    rb_ = nullptr;
  }
}

void MiniaudioOutput::write_audio(MediaFrame const& frame) {
  if (!rb_ || !frame.data || !frame.is_audio) return;
  
  std::uint32_t frames_to_write = frame.num_samples;
  const float* pData = reinterpret_cast<const float*>(frame.data);

  while (frames_to_write > 0) {
      std::uint32_t frames_written = frames_to_write;
      void* pWriteBuffer;
      
      ma_pcm_rb_acquire_write(rb_, &frames_written, &pWriteBuffer);
      if (frames_written > 0) {
          memcpy(pWriteBuffer, pData, frames_written * frame.channels * sizeof(float));
          ma_pcm_rb_commit_write(rb_, frames_written);
          pData += frames_written * frame.channels;
          frames_to_write -= frames_written;
      } else {
          // Buffer full, wait a bit so we don't spin lock
          ma_sleep(1);
      }
  }
}

void MiniaudioOutput::set_volume(float gain) {
  if (device_) {
    ma_device_set_master_volume(device_, gain);
  }
}

void MiniaudioOutput::data_callback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, std::uint32_t frameCount) {
  MiniaudioOutput* self = static_cast<MiniaudioOutput*>(pDevice->pUserData);
  if (!self->rb_) return;

  std::uint32_t frames_read = frameCount;
  void* pReadBuffer;
  ma_pcm_rb_acquire_read(self->rb_, &frames_read, &pReadBuffer);
  
  if (frames_read > 0) {
      memcpy(pOutput, pReadBuffer, frames_read * pDevice->playback.channels * sizeof(float));
      ma_pcm_rb_commit_read(self->rb_, frames_read);
  } else {
      memset(pOutput, 0, frameCount * pDevice->playback.channels * sizeof(float));
  }
  
  if (frames_read < frameCount) {
      float* out_f32 = static_cast<float*>(pOutput);
      memset(out_f32 + (frames_read * pDevice->playback.channels), 0, (frameCount - frames_read) * pDevice->playback.channels * sizeof(float));
  }
}

} // namespace media::audio
