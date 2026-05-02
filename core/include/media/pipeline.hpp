#pragma once

#include "media/types.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace media {

struct StreamInfo {
  void* codec_parameters{nullptr}; // AVCodecParameters*
  int stream_index{-1};
  double time_base{0.0};
};

struct IPacketSource {
  virtual ~IPacketSource() = default;
  virtual bool open(MediaUri const& uri) = 0;
  virtual bool read_packet(MediaPacket& pkt) = 0;
  virtual void free_packet(MediaPacket& pkt) = 0;
  
  virtual StreamInfo video_info() const = 0;
  virtual StreamInfo audio_info() const = 0;
};

struct IDecoder {
  virtual ~IDecoder() = default;
  virtual bool open(StreamInfo const& info) = 0;
  virtual bool send_packet(MediaPacket const& pkt) = 0;
  virtual bool receive_frame(MediaFrame& frame) = 0;
  virtual void free_frame(MediaFrame& frame) = 0;
};

struct IAudioOutput {
  virtual ~IAudioOutput() = default;
  virtual bool open(int sample_rate, int channels) = 0;
  virtual void write_audio(MediaFrame const& frame) = 0;
};

struct IVideoOutput {
  virtual ~IVideoOutput() = default;
  virtual void render_frame(MediaFrame const& frame) = 0;
};

// Owns graph wiring and flush/seek barriers (v1: holds collaborators only).
class MediaPipeline {
public:
  MediaPipeline();
  ~MediaPipeline();

  void attach_uri(MediaUri uri);
  void clear();

  void set_packet_source(std::shared_ptr<IPacketSource> source);
  void set_video_decoder(std::shared_ptr<IDecoder> decoder);
  void set_audio_decoder(std::shared_ptr<IDecoder> decoder);
  void set_audio_output(std::shared_ptr<IAudioOutput> audio);
  void set_video_output(std::shared_ptr<IVideoOutput> video);

  std::shared_ptr<IPacketSource> source() const { return source_; }
  std::shared_ptr<IDecoder> video_decoder() const { return video_decoder_; }
  std::shared_ptr<IDecoder> audio_decoder() const { return audio_decoder_; }
  std::shared_ptr<IAudioOutput> audio_output() const { return audio_; }
  std::shared_ptr<IVideoOutput> video_output() const { return video_; }

  MediaUri uri() const;

private:
  mutable std::mutex mutex_{};
  MediaUri uri_{};
  std::shared_ptr<IPacketSource> source_{};
  std::shared_ptr<IDecoder> video_decoder_{};
  std::shared_ptr<IDecoder> audio_decoder_{};
  std::shared_ptr<IAudioOutput> audio_{};
  std::shared_ptr<IVideoOutput> video_{};
};


} // namespace media
