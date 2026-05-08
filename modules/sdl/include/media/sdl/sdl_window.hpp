#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

namespace media::sdl {

// RAII wrapper around a SDL_Window + SDL_Renderer pair.
class SdlWindow {
public:
    SdlWindow() = default;
    ~SdlWindow() { destroy(); }

    SdlWindow(SdlWindow const&)            = delete;
    SdlWindow& operator=(SdlWindow const&) = delete;

    // Creates a hardware-accelerated renderer window.
    bool create(const char* title, int width, int height);

    // Destroys the window and renderer.
    void destroy();

    SDL_Window*   window()   const { return window_; }
    SDL_Renderer* renderer() const { return renderer_; }

    // Returns current drawable size (accounts for HiDPI / Retina).
    void get_size(int& out_w, int& out_h) const;

    // Toggle between windowed and borderless fullscreen.
    void toggle_fullscreen();
    bool is_fullscreen() const { return fullscreen_; }

private:
    SDL_Window*   window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    bool          fullscreen_{false};
};

} // namespace media::sdl
