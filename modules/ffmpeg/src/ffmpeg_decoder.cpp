#include "ffmpeg_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace media::ffmpeg {

FFmpegDecoder::FFmpegDecoder() = default;

FFmpegDecoder::~FFmpegDecoder() {
  if (codec_ctx_) {
    avcodec_free_context(&codec_ctx_);
  }
}

bool FFmpegDecoder::open(StreamInfo const& info) {
  if (!info.codec_parameters) return false;
  
  AVCodecParameters* params = static_cast<AVCodecParameters*>(info.codec_parameters);
  const AVCodec* codec = avcodec_find_decoder(params->codec_id);
  if (!codec) return false;
  
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (!codec_ctx_) return false;
  
  if (avcodec_parameters_to_context(codec_ctx_, params) < 0) {
    return false;
  }
  
  if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
    return false;
  }
  
  time_base_ = info.time_base;
  return true;
}

bool FFmpegDecoder::send_packet(MediaPacket const& pkt) {
  if (!codec_ctx_) return false;
  AVPacket* avpkt = static_cast<AVPacket*>(pkt.opaque);
  return avcodec_send_packet(codec_ctx_, avpkt) == 0;
}

bool FFmpegDecoder::receive_frame(MediaFrame& frame) {
  if (!codec_ctx_) return false;
  
  AVFrame* avframe = av_frame_alloc();
  if (!avframe) return false;
  
  int ret = avcodec_receive_frame(codec_ctx_, avframe);
  if (ret == 0) {
    frame.opaque = avframe;
    frame.width = avframe->width;
    frame.height = avframe->height;
    frame.pts_seconds = avframe->pts * time_base_;
    return true;
  }
  
  av_frame_free(&avframe);
  return false;
}

void FFmpegDecoder::free_frame(MediaFrame& frame) {
  if (frame.opaque) {
    AVFrame* avframe = static_cast<AVFrame*>(frame.opaque);
    av_frame_free(&avframe);
    frame.opaque = nullptr;
  }
}

} // namespace media::ffmpeg
