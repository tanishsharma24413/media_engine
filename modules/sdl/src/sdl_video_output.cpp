#include "media/sdl/sdl_video_output.hpp"

#include <cstring>

namespace media::sdl {

// ── static event type ────────────────────────────────────────────────────────

Uint32 SdlVideoOutput::frame_event_type() {
    // SDL_RegisterEvents is thread-safe and idempotent after the first call.
    static const Uint32 s_type = SDL_RegisterEvents(1);
    return s_type;
}

// ── lifecycle ────────────────────────────────────────────────────────────────

SdlVideoOutput::SdlVideoOutput() = default;

SdlVideoOutput::~SdlVideoOutput() { destroy(); }

void SdlVideoOutput::set_renderer(SDL_Renderer* r) {
    renderer_ = r;
}

void SdlVideoOutput::destroy() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    tex_w_ = tex_h_ = 0;
    renderer_ = nullptr;
}

// ── engine thread ────────────────────────────────────────────────────────────

void SdlVideoOutput::render_frame(MediaFrame const& frame) {
    if (!frame.data || !frame.is_video || frame.width <= 0 || frame.height <= 0)
        return;

    const int row_bytes = frame.width * 4;  // RGBA

    {
        std::lock_guard<std::mutex> lock(buf_mutex_);
        pixel_buf_.resize(static_cast<std::size_t>(frame.height * row_bytes));

        // Copy row-by-row to strip any padding in frame.pitch.
        for (int y = 0; y < frame.height; ++y) {
            std::memcpy(
                pixel_buf_.data() + y * row_bytes,
                frame.data         + y * frame.pitch,
                static_cast<std::size_t>(row_bytes));
        }
        pending_w_     = frame.width;
        pending_h_     = frame.height;
        pending_pitch_ = row_bytes;
    }

    video_w_.store(frame.width);
    video_h_.store(frame.height);
    frame_ready_.store(true);

    // Wake up the main SDL event loop.
    SDL_Event ev{};
    ev.type = frame_event_type();
    SDL_PushEvent(&ev);
}

// ── main thread ──────────────────────────────────────────────────────────────

bool SdlVideoOutput::present(SDL_Rect const& dst_rect) {
    if (!frame_ready_.exchange(false))
        return false;
    if (!renderer_)
        return false;

    int pw, ph, pp;
    {
        std::lock_guard<std::mutex> lock(buf_mutex_);
        pw = pending_w_;
        ph = pending_h_;
        pp = pending_pitch_;

        // (Re)create texture when dimensions change.
        if (!texture_ || tex_w_ != pw || tex_h_ != ph) {
            if (texture_) SDL_DestroyTexture(texture_);
            texture_ = SDL_CreateTexture(
                renderer_,
                SDL_PIXELFORMAT_RGBA32,
                SDL_TEXTUREACCESS_STREAMING,
                pw, ph);
            if (!texture_) return false;
            tex_w_ = pw;
            tex_h_ = ph;
        }

        SDL_UpdateTexture(texture_, nullptr, pixel_buf_.data(), pp);
    }

    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);
    return true;
}

} // namespace media::sdl
