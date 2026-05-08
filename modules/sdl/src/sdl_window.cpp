#include "media/sdl/sdl_window.hpp"

namespace media::sdl {

bool SdlWindow::create(const char* title, int width, int height) {
    destroy();

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window_) return false;

    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer_) {
        // Fallback: software renderer (Raspberry Pi with no DRM/KMS driver, etc.)
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    return true;
}

void SdlWindow::destroy() {
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_)   { SDL_DestroyWindow(window_);     window_   = nullptr; }
    fullscreen_ = false;
}

void SdlWindow::get_size(int& out_w, int& out_h) const {
    if (window_) {
        SDL_GetWindowSize(window_, &out_w, &out_h);
    } else {
        out_w = out_h = 0;
    }
}

void SdlWindow::toggle_fullscreen() {
    if (!window_) return;
    fullscreen_ = !fullscreen_;
    SDL_SetWindowFullscreen(
        window_,
        fullscreen_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

} // namespace media::sdl
