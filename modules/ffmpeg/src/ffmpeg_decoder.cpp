#include "ffmpeg_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace media::ffmpeg {

FFmpegDecoder::FFmpegDecoder() = default;

FFmpegDecoder::~FFmpegDecoder() {
  if (sws_ctx_) sws_freeContext(sws_ctx_);
  if (codec_ctx_) avcodec_free_context(&codec_ctx_);
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
    AVFrame* rgb_frame = av_frame_alloc();
    rgb_frame->format = AV_PIX_FMT_RGBA;
    rgb_frame->width = avframe->width;
    rgb_frame->height = avframe->height;
    av_frame_get_buffer(rgb_frame, 32);

    sws_ctx_ = sws_getCachedContext(sws_ctx_,
                                    avframe->width, avframe->height, (AVPixelFormat)avframe->format,
                                    rgb_frame->width, rgb_frame->height, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);

    sws_scale(sws_ctx_, avframe->data, avframe->linesize, 0, avframe->height,
              rgb_frame->data, rgb_frame->linesize);

    rgb_frame->pts = avframe->pts;
    av_frame_free(&avframe);

    frame.opaque = rgb_frame;
    frame.data = rgb_frame->data[0];
    frame.pitch = rgb_frame->linesize[0];
    frame.width = rgb_frame->width;
    frame.height = rgb_frame->height;
    frame.pts_seconds = rgb_frame->pts * time_base_;
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
    frame.data = nullptr;
  }
}

} // namespace media::ffmpeg
