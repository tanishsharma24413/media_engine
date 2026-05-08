#pragma once

#include "media/pipeline.hpp"

#include <SDL2/SDL.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace media::sdl {

// Thread-safe SDL2 video output.
//
// render_frame()  — called from the engine's vout thread. Copies pixel data
//                   into a CPU buffer and posts a custom SDL event to wake
//                   the main thread.
// present()       — called from the SDL main thread inside the event loop.
//                   Uploads pixels to a GPU texture and renders it letterboxed
//                   into dst_rect.
class SdlVideoOutput final : public IVideoOutput {
public:
    SdlVideoOutput();
    ~SdlVideoOutput() override;

    // Must be called from the main thread before playback starts.
    void set_renderer(SDL_Renderer* renderer);

    // IVideoOutput — called from engine vout thread.
    void render_frame(MediaFrame const& frame) override;

    // Present the latest frame into dst_rect.  Returns true if a frame was
    // uploaded.  Must be called from the SDL main thread.
    bool present(SDL_Rect const& dst_rect);

    // Returns true once at least one frame has arrived.
    bool has_video() const { return video_w_.load() > 0; }
    int  video_width()  const { return video_w_.load(); }
    int  video_height() const { return video_h_.load(); }

    // SDL user-event type registered at startup.
    // The main event loop should check for this to trigger a present().
    static Uint32 frame_event_type();

    // Release GPU resources (call from main thread before destroying renderer).
    void destroy();

private:
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture*  texture_{nullptr};
    int           tex_w_{0}, tex_h_{0};

    std::mutex           buf_mutex_;
    std::vector<uint8_t> pixel_buf_;
    int pending_w_{0}, pending_h_{0}, pending_pitch_{0};

    std::atomic<bool> frame_ready_{false};
    std::atomic<int>  video_w_{0};
    std::atomic<int>  video_h_{0};
};

} // namespace media::sdl
