#pragma once

#include "media/pipeline.hpp"

struct ma_device;
struct ma_pcm_rb;

namespace media::audio {

class MiniaudioOutput final : public IAudioOutput {
public:
  MiniaudioOutput();
  ~MiniaudioOutput() override;

  bool open(int sample_rate, int channels) override;
  void shutdown();

  void write_audio(MediaFrame const& frame) override;

private:
  static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, std::uint32_t frameCount);

  ma_device* device_{nullptr};
  ma_pcm_rb* rb_{nullptr};
};

} // namespace media::audio
