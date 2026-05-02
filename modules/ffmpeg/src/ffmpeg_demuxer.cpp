#include "ffmpeg_demuxer.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

namespace media::ffmpeg {

FFmpegDemuxer::FFmpegDemuxer() = default;

FFmpegDemuxer::~FFmpegDemuxer() {
  if (format_ctx_) {
    avformat_close_input(&format_ctx_);
  }
}

bool FFmpegDemuxer::open(MediaUri const& uri) {
  if (format_ctx_) {
    avformat_close_input(&format_ctx_);
  }
  
  if (avformat_open_input(&format_ctx_, uri.value.c_str(), nullptr, nullptr) != 0) {
    return false;
  }
  
  if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
    return false;
  }
  
  for (unsigned int i = 0; i < format_ctx_->nb_streams; i++) {
    if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index_ < 0) {
      video_stream_index_ = i;
    } else if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0) {
      audio_stream_index_ = i;
    }
  }
  
  return video_stream_index_ >= 0 || audio_stream_index_ >= 0;
}

bool FFmpegDemuxer::read_packet(MediaPacket& pkt) {
  if (!format_ctx_) return false;
  
  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt) return false;
  
  if (av_read_frame(format_ctx_, avpkt) >= 0) {
    pkt.opaque = avpkt;
    pkt.is_video = (avpkt->stream_index == video_stream_index_);
    pkt.is_audio = (avpkt->stream_index == audio_stream_index_);
    
    if (pkt.is_video || pkt.is_audio) {
      AVStream* st = format_ctx_->streams[avpkt->stream_index];
      pkt.pts_seconds = avpkt->pts * av_q2d(st->time_base);
      return true;
    } else {
      av_packet_free(&avpkt);
      return read_packet(pkt); // recurse to read next
    }
  }
  
  av_packet_free(&avpkt);
  return false;
}

void FFmpegDemuxer::free_packet(MediaPacket& pkt) {
  if (pkt.opaque) {
    AVPacket* avpkt = static_cast<AVPacket*>(pkt.opaque);
    av_packet_free(&avpkt);
    pkt.opaque = nullptr;
  }
}

StreamInfo FFmpegDemuxer::video_info() const {
  if (video_stream_index_ >= 0 && format_ctx_) {
    AVStream* st = format_ctx_->streams[video_stream_index_];
    return {st->codecpar, video_stream_index_, av_q2d(st->time_base)};
  }
  return {};
}

StreamInfo FFmpegDemuxer::audio_info() const {
  if (audio_stream_index_ >= 0 && format_ctx_) {
    AVStream* st = format_ctx_->streams[audio_stream_index_];
    return {st->codecpar, audio_stream_index_, av_q2d(st->time_base)};
  }
  return {};
}

} // namespace media::ffmpeg
