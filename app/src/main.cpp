// Tanish Player v1.0
// Cross-platform media player built on the libtanish engine.
// Platforms: Windows 10/11 · Linux x86_64/ARM64 · Raspberry Pi 4/5

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <Windows.h>
#  include <commdlg.h>
#endif

#include <SDL2/SDL.h>
#ifdef TANISH_HAS_SDL_TTF
#  include <SDL2/SDL_ttf.h>
#endif

#include "media/player.hpp"
#include "media/pipeline.hpp"
#include "media/modules/ffmpeg/ffmpeg_backend.hpp"
#include "media/sdl/sdl_video_output.hpp"
#include "media/sdl/sdl_window.hpp"
#include "../../modules/audio/src/miniaudio_output.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

// ─── Colour palette ──────────────────────────────────────────────────────────
static constexpr SDL_Color C_BG        = {0x0D,0x0D,0x12,0xFF};
static constexpr SDL_Color C_TOOLBAR   = {0x08,0x08,0x0E,0xFF};
static constexpr SDL_Color C_STATUS    = {0x05,0x05,0x09,0xFF};
static constexpr SDL_Color C_ACCENT    = {0x7C,0x5A,0xF0,0xFF};
static constexpr SDL_Color C_ACCENT2   = {0x4A,0x35,0x99,0xFF};
static constexpr SDL_Color C_TEXT      = {0xE8,0xE8,0xF0,0xFF};
static constexpr SDL_Color C_TEXTDIM   = {0x70,0x70,0x90,0xFF};
static constexpr SDL_Color C_SEEKBG    = {0x1A,0x1A,0x28,0xFF};
static constexpr SDL_Color C_BTN      = {0x14,0x14,0x20,0xFF};
static constexpr SDL_Color C_BTNHOV   = {0x20,0x18,0x38,0xFF};

// ─── Layout ──────────────────────────────────────────────────────────────────
static constexpr int SEEK_H    = 6;
static constexpr int TOOLBAR_H = 54;
static constexpr int STATUS_H  = 22;
static constexpr int UI_H      = SEEK_H + TOOLBAR_H + STATUS_H;
static constexpr int BTN_W     = 44;
static constexpr int BTN_H     = 36;

// ─── Drawing helpers ─────────────────────────────────────────────────────────
static void set_col(SDL_Renderer* r, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}
static void fill(SDL_Renderer* r, SDL_Rect rc, SDL_Color c) {
    set_col(r, c); SDL_RenderFillRect(r, &rc);
}
static bool in_rect(int x, int y, SDL_Rect rc) {
    return x >= rc.x && x < rc.x+rc.w && y >= rc.y && y < rc.y+rc.h;
}

// Letterbox: fit video inside area preserving aspect ratio.
static SDL_Rect letterbox(int vw, int vh, SDL_Rect area) {
    if (vw <= 0 || vh <= 0) return area;
    float sx = (float)area.w / vw, sy = (float)area.h / vh;
    float s  = std::min(sx, sy);
    int rw = (int)(vw * s), rh = (int)(vh * s);
    return {area.x + (area.w - rw)/2, area.y + (area.h - rh)/2, rw, rh};
}

static std::string fmt_time(double sec) {
    if (sec < 0) sec = 0;
    int s = (int)sec, m = s/60; s %= 60;
    int h = m/60;               m %= 60;
    char buf[32];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else        std::snprintf(buf, sizeof(buf), "%02d:%02d",    m, s);
    return buf;
}

static const char* state_label(media::PlayerState s) {
    switch (s) {
        case media::PlayerState::Playing:  return "▶  Playing";
        case media::PlayerState::Paused:   return "⏸  Paused";
        case media::PlayerState::Stopped:  return "⏹  Stopped";
        case media::PlayerState::Opening:  return "⏳  Opening…";
        case media::PlayerState::Error:    return "⚠  Error";
        default:                           return "Tanish Player";
    }
}

// ─── File-open dialog ────────────────────────────────────────────────────────
static std::string open_dialog() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter =
        L"Media Files\0*.mp4;*.mkv;*.avi;*.mov;*.flv;*.webm;*.ts;"
        L"*.mp3;*.aac;*.flac;*.ogg;*.wav;*.opus\0All Files\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile  = MAX_PATH;
    ofn.Flags     = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return {};
    std::string out(n-1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buf, -1, out.data(), n, nullptr, nullptr);
    return out;
#else
    // Try zenity, then kdialog.
    for (const char* cmd : {
            "zenity --file-selection --title='Open Media' 2>/dev/null",
            "kdialog --getopenfilename . '*.mp4 *.mkv *.avi *.mp3' 2>/dev/null"}) {
        FILE* f = popen(cmd, "r");
        if (!f) continue;
        char line[4096]{};
        bool got = fgets(line, sizeof(line), f);
        pclose(f);
        if (got && line[0]) {
            std::string s(line);
            while (!s.empty() && (s.back()=='\n'||s.back()=='\r')) s.pop_back();
            if (!s.empty()) return s;
        }
    }
    return {};
#endif
}

// ─── Text rendering ──────────────────────────────────────────────────────────
#ifdef TANISH_HAS_SDL_TTF
static TTF_Font* g_font_sm = nullptr;   // 11 pt — status bar
static TTF_Font* g_font_md = nullptr;   // 14 pt — toolbar labels
static TTF_Font* g_font_lg = nullptr;   // 18 pt — button glyphs

static TTF_Font* load_font(int pt) {
    static const char* paths[] = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
#endif
        nullptr
    };
    for (int i = 0; paths[i]; ++i) {
        TTF_Font* f = TTF_OpenFont(paths[i], pt);
        if (f) return f;
    }
    return nullptr;
}

struct TextCache {
    SDL_Texture* tex{nullptr};
    int w{0}, h{0};
    std::string last;

    void update(SDL_Renderer* r, TTF_Font* f, const char* txt, SDL_Color col) {
        if (!f || !txt) return;
        if (txt == last) return;
        last = txt;
        if (tex) { SDL_DestroyTexture(tex); tex = nullptr; w = h = 0; }
        SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt, col);
        if (!s) return;
        tex = SDL_CreateTextureFromSurface(r, s);
        SDL_FreeSurface(s);
        if (tex) SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    }

    void draw(SDL_Renderer* r, int x, int y) const {
        if (!tex) return;
        SDL_Rect d{x, y, w, h};
        SDL_RenderCopy(r, tex, nullptr, &d);
    }

    void draw_centered(SDL_Renderer* r, SDL_Rect area) const {
        if (!tex) return;
        SDL_Rect d{area.x+(area.w-w)/2, area.y+(area.h-h)/2, w, h};
        SDL_RenderCopy(r, tex, nullptr, &d);
    }

    void destroy() { if (tex) { SDL_DestroyTexture(tex); tex = nullptr; } }
};
#endif // TANISH_HAS_SDL_TTF

// ─── AppState ─────────────────────────────────────────────────────────────────
struct AppState {
    // Engine
    media::sdl::SdlWindow   win;
    media::sdl::SdlVideoOutput vout;
    std::shared_ptr<media::audio::MiniaudioOutput>
        aout{std::make_shared<media::audio::MiniaudioOutput>()};
    media::Player player;

    // Playback state
    media::PlayerState pstate{media::PlayerState::Idle};
    std::string filepath;
    std::string filename;   // basename only

    // UI state
    float volume{1.0f};
    bool  vol_drag{false};
    int   mx{0}, my{0};    // last mouse pos

    // Layout rects (recalculated on resize)
    int ww{1280}, wh{720};
    SDL_Rect area_video{};
    SDL_Rect area_seek{};
    SDL_Rect area_toolbar{};
    SDL_Rect area_status{};
    SDL_Rect btn_play{};
    SDL_Rect btn_stop{};
    SDL_Rect btn_open{};
    SDL_Rect vol_track{};
    SDL_Rect time_area{};

#ifdef TANISH_HAS_SDL_TTF
    TextCache tc_play, tc_stop, tc_open;
    TextCache tc_time, tc_status, tc_vol_icon;
#endif

    void layout() {
        area_video   = {0, 0,          ww, wh - UI_H};
        area_seek    = {0, wh - UI_H,  ww, SEEK_H};
        area_toolbar = {0, wh - UI_H + SEEK_H, ww, TOOLBAR_H};
        area_status  = {0, wh - STATUS_H, ww, STATUS_H};

        int ty = area_toolbar.y + (TOOLBAR_H - BTN_H) / 2;
        int bx = 14;
        btn_play = {bx, ty, BTN_W, BTN_H}; bx += BTN_W + 6;
        btn_stop = {bx, ty, BTN_W, BTN_H}; bx += BTN_W + 18;

        // time display
        time_area = {bx, area_toolbar.y, 120, TOOLBAR_H};

        // open button — right-aligned
        btn_open = {ww - BTN_W*2 - 14, ty, BTN_W*2, BTN_H};

        // volume track
        int vx = btn_open.x - 130 - 10;
        vol_track = {vx, area_toolbar.y + TOOLBAR_H/2 - 3, 118, 6};
    }

    SDL_Rect vol_thumb_rect() const {
        int tx = vol_track.x + (int)(volume * vol_track.w) - 8;
        return {tx, area_toolbar.y + TOOLBAR_H/2 - 9, 18, 18};
    }

    void open(const std::string& path) {
        if (path.empty()) return;
        filepath = path;
        // Extract basename
        auto p = path.find_last_of("/\\");
        filename = (p != std::string::npos) ? path.substr(p+1) : path;
        player.stop();
        player.open(media::MediaUri{path});
        player.play();
    }
};

// ─── Draw one toolbar button ──────────────────────────────────────────────────
static void draw_btn(SDL_Renderer* r, SDL_Rect rc, const char* label,
                     bool hover, bool active
#ifdef TANISH_HAS_SDL_TTF
                     , TextCache& tc, TTF_Font* font
#endif
                    )
{
    SDL_Color bg = active ? C_ACCENT2 : (hover ? C_BTNHOV : C_BTN);
    fill(r, rc, bg);

    // Thin accent border on hover
    if (hover || active) {
        set_col(r, C_ACCENT);
        SDL_RenderDrawRect(r, &rc);
    }

#ifdef TANISH_HAS_SDL_TTF
    tc.update(r, font, label, hover ? C_TEXT : C_TEXTDIM);
    tc.draw_centered(r, rc);
#else
    (void)label;
#endif
}

// ─── Draw full UI ─────────────────────────────────────────────────────────────
static void draw(AppState& app) {
    SDL_Renderer* r = app.win.renderer();

    // ── Video background
    fill(r, app.area_video, C_BG);

    // ── Video frame (letterboxed)
    {
        SDL_Rect dst = letterbox(app.vout.video_width(), app.vout.video_height(),
                                 app.area_video);
        app.vout.present(dst);
    }

    // ── Seek bar
    fill(r, app.area_seek, C_SEEKBG);
    double pos = app.player.position_seconds();
    double dur = app.player.duration_seconds();
    if (dur > 0) {
        float frac = (float)std::clamp(pos / dur, 0.0, 1.0);
        SDL_Rect filled{app.area_seek.x, app.area_seek.y,
                        (int)(frac * app.ww), SEEK_H};
        fill(r, filled, C_ACCENT);
        // Thumb dot
        int tx = filled.x + filled.w - 3;
        SDL_Rect dot{tx, app.area_seek.y - 2, 6, SEEK_H + 4};
        fill(r, dot, C_ACCENT);
    }

    // ── Toolbar background
    fill(r, app.area_toolbar, C_TOOLBAR);
    // Separator line
    set_col(r, C_ACCENT2);
    SDL_RenderDrawLine(r, 0, app.area_seek.y, app.ww, app.area_seek.y);

    bool playing = (app.pstate == media::PlayerState::Playing);

    // ── Play/Pause button
    bool hov_play = in_rect(app.mx, app.my, app.btn_play);
    draw_btn(r, app.btn_play, playing ? "⏸" : "▶",
             hov_play, playing
#ifdef TANISH_HAS_SDL_TTF
             , app.tc_play, g_font_lg
#endif
            );

    // ── Stop button
    bool hov_stop = in_rect(app.mx, app.my, app.btn_stop);
    draw_btn(r, app.btn_stop, "⏹",
             hov_stop, false
#ifdef TANISH_HAS_SDL_TTF
             , app.tc_stop, g_font_lg
#endif
            );

    // ── Time display
    {
        std::string t = fmt_time(pos) + " / " + fmt_time(dur);
#ifdef TANISH_HAS_SDL_TTF
        app.tc_time.update(r, g_font_sm, t.c_str(), C_TEXTDIM);
        app.tc_time.draw_centered(r, app.time_area);
#else
        (void)t;
#endif
    }

    // ── Volume track
    fill(r, app.vol_track, C_SEEKBG);
    {
        SDL_Rect filled{app.vol_track.x, app.vol_track.y,
                        (int)(app.volume * app.vol_track.w), app.vol_track.h};
        fill(r, filled, C_ACCENT2);
    }
    // Volume thumb
    {
        SDL_Rect th = app.vol_thumb_rect();
        bool hov_vol = in_rect(app.mx, app.my, th) || app.vol_drag;
        fill(r, th, hov_vol ? C_ACCENT : C_ACCENT2);
    }

    // ── Open button
    bool hov_open = in_rect(app.mx, app.my, app.btn_open);
    draw_btn(r, app.btn_open, "Open File",
             hov_open, false
#ifdef TANISH_HAS_SDL_TTF
             , app.tc_open, g_font_md
#endif
            );

    // ── Status bar
    fill(r, app.area_status, C_STATUS);
    set_col(r, C_ACCENT2);
    SDL_RenderDrawLine(r, 0, app.area_status.y, app.ww, app.area_status.y);
    {
        std::string label = std::string(state_label(app.pstate));
        if (!app.filename.empty()) label += "  |  " + app.filename;
#ifdef TANISH_HAS_SDL_TTF
        app.tc_status.update(r, g_font_sm, label.c_str(), C_TEXTDIM);
        app.tc_status.draw(r,
            app.area_status.x + 10,
            app.area_status.y + (STATUS_H - app.tc_status.h) / 2);
#else
        (void)label;
#endif
    }

    SDL_RenderPresent(r);
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

#ifdef TANISH_HAS_SDL_TTF
    if (TTF_Init() == 0) {
        g_font_sm = load_font(11);
        g_font_md = load_font(14);
        g_font_lg = load_font(18);
    }
#endif

    AppState app;

    if (!app.win.create("Tanish Player", 1280, 720)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 1;
    }
    app.win.get_size(app.ww, app.wh);
    app.layout();

    app.vout.set_renderer(app.win.renderer());

    // ── Build pipeline
    {
        auto pipeline = std::make_shared<media::MediaPipeline>();
        pipeline->set_packet_source(media::ffmpeg::create_ffmpeg_demuxer());
        pipeline->set_video_decoder(media::ffmpeg::create_ffmpeg_decoder());
        pipeline->set_audio_decoder(media::ffmpeg::create_ffmpeg_decoder());
        pipeline->set_video_output(
            std::shared_ptr<media::IVideoOutput>(&app.vout, [](auto*){}));
        pipeline->set_audio_output(app.aout);
        app.player.set_pipeline(pipeline);
    }

    // ── State callback
    app.player.set_on_state([&app](media::PlayerState s) {
        app.pstate = s;
        if (s == media::PlayerState::Playing) {
            app.aout->open(44100, 2);
        }
    });
    app.player.set_on_error([](std::string const& e) {
        SDL_Log("[libtanish] error: %s", e.c_str());
    });

    // ── Open file from command line
    if (argc >= 2) app.open(argv[1]);

    const Uint32 FRAME_EVT = media::sdl::SdlVideoOutput::frame_event_type();

    bool running = true;
    SDL_Event ev;

    while (running) {
        // Block until event; wake every 100 ms to refresh time display.
        SDL_WaitEventTimeout(&ev, 100);

        do {
            if (ev.type == SDL_QUIT) { running = false; break; }

            if (ev.type == FRAME_EVT) {
                // New video frame ready — draw is called at end of loop.
            }

            if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                    ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app.win.get_size(app.ww, app.wh);
                    app.layout();
                }
            }

            if (ev.type == SDL_DROPFILE) {
                std::string path(ev.drop.file);
                SDL_free(ev.drop.file);
                app.open(path);
            }

            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                case SDLK_SPACE:
                    if (app.pstate == media::PlayerState::Playing)
                        app.player.pause();
                    else
                        app.player.play();
                    break;
                case SDLK_ESCAPE: app.player.stop(); break;
                case SDLK_f:
                case SDLK_F11:    app.win.toggle_fullscreen(); break;
                case SDLK_o:
                    if (ev.key.keysym.mod & KMOD_CTRL) {
                        auto p = open_dialog();
                        if (!p.empty()) app.open(p);
                    }
                    break;
                default: break;
                }
            }

            if (ev.type == SDL_MOUSEMOTION) {
                app.mx = ev.motion.x;
                app.my = ev.motion.y;
                if (app.vol_drag) {
                    float f = (float)(app.mx - app.vol_track.x) / app.vol_track.w;
                    app.volume = std::clamp(f, 0.0f, 1.0f);
                    app.player.set_volume(app.volume);
                }
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int bx = ev.button.x, by = ev.button.y;
                if (in_rect(bx, by, app.btn_play)) {
                    if (app.pstate == media::PlayerState::Playing)
                        app.player.pause();
                    else
                        app.player.play();
                }
                else if (in_rect(bx, by, app.btn_stop)) {
                    app.player.stop();
                }
                else if (in_rect(bx, by, app.btn_open)) {
                    auto p = open_dialog();
                    if (!p.empty()) app.open(p);
                }
                else if (in_rect(bx, by, app.vol_thumb_rect()) ||
                         in_rect(bx, by, app.vol_track)) {
                    app.vol_drag = true;
                    float f = (float)(bx - app.vol_track.x) / app.vol_track.w;
                    app.volume = std::clamp(f, 0.0f, 1.0f);
                    app.player.set_volume(app.volume);
                }
            }

            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                app.vol_drag = false;
            }
        } while (SDL_PollEvent(&ev));

        if (!running) break;
        draw(app);
    }

    // ── Clean shutdown
    app.player.stop();
    app.vout.destroy();
    app.aout->shutdown();

#ifdef TANISH_HAS_SDL_TTF
    app.tc_play.destroy(); app.tc_stop.destroy();
    app.tc_open.destroy(); app.tc_time.destroy();
    app.tc_status.destroy();
    if (g_font_sm) TTF_CloseFont(g_font_sm);
    if (g_font_md) TTF_CloseFont(g_font_md);
    if (g_font_lg) TTF_CloseFont(g_font_lg);
    TTF_Quit();
#endif

    SDL_Quit();
    return 0;
}
