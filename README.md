# Tanish Player — libtanish Media Engine

A cross-platform, hardware-accelerated media player built on the **libtanish** engine.

Supports: **Windows 10/11** · **Linux x86_64 / ARM64** · **Raspberry Pi 4/5**

---

## Quick-start

### Windows (MSVC + vcpkg)

```powershell
# 1. Install dependencies
vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows

# 2. Configure
cmake -B build -S . `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DMEDIA_ENABLE_FFMPEG=ON `
  -DFFMPEG_ROOT="C:/path/to/ffmpeg"

# 3. Build
cmake --build build --config Release --target TanishPlayer

# 4. Run
.\build\app\Release\TanishPlayer.exe
# Or pass a file on the command line:
.\build\app\Release\TanishPlayer.exe "C:\Videos\movie.mp4"
```

> **FFmpeg note:** `FFMPEG_ROOT` must contain `include/` and `lib/` with
> `avformat`, `avcodec`, `avutil`, `swscale`, `swresample`.
> Pre-built Windows binaries: <https://github.com/BtbN/FFmpeg-Builds/releases>

---

### Linux (Debian / Ubuntu)

```bash
# 1. Install dependencies
sudo apt install \
  libsdl2-dev libsdl2-ttf-dev \
  libavformat-dev libavcodec-dev \
  libavutil-dev libswscale-dev libswresample-dev

# 2. Configure + build
cmake -B build -S . -DMEDIA_ENABLE_FFMPEG=ON
cmake --build build --target TanishPlayer

# 3. Run
./build/app/TanishPlayer
./build/app/TanishPlayer /path/to/video.mp4
```

---

### Raspberry Pi 4/5 — native build

```bash
# Same as Linux above (Raspberry Pi OS is Debian-based).
sudo apt install \
  libsdl2-dev libsdl2-ttf-dev \
  libavformat-dev libavcodec-dev \
  libavutil-dev libswscale-dev libswresample-dev

cmake -B build -S . -DMEDIA_ENABLE_FFMPEG=ON
cmake --build build --target TanishPlayer -j$(nproc)
./build/app/TanishPlayer
```

### Raspberry Pi — cross-compile from x64 Linux

```bash
sudo apt install \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
  libsdl2-dev:arm64 libavformat-dev:arm64 \
  libavcodec-dev:arm64 libavutil-dev:arm64 \
  libswscale-dev:arm64 libswresample-dev:arm64

cmake -B build-rpi -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/rpi-aarch64.cmake \
  -DMEDIA_ENABLE_FFMPEG=ON
cmake --build build-rpi --target TanishPlayer -j$(nproc)
```

---

## Controls

| Key / Action | Effect |
|---|---|
| `Space` | Play / Pause |
| `Escape` | Stop |
| `F` or `F11` | Toggle fullscreen |
| `Ctrl+O` | Open file dialog |
| Click **Open File** | Open file dialog |
| Drag & drop file | Open immediately |
| Drag volume slider | Adjust audio gain |

---

## Architecture

```
libtanish/
├── core/               media::Player · Engine · Pipeline · Clock
├── modules/
│   ├── ffmpeg/         IPacketSource + IDecoder via libavformat/avcodec
│   ├── audio/          IAudioOutput via miniaudio (WASAPI/ALSA/PulseAudio)
│   ├── sdl/            IVideoOutput + SdlWindow via SDL2 (all platforms)
│   └── win/            D3D11Presenter — optional high-perf path (Windows only)
├── app/                TanishPlayer — SDL2 UI (single source, all platforms)
└── cmake/
    ├── MediaFFmpeg.cmake
    ├── FindSDL2.cmake
    └── rpi-aarch64.cmake
```

## Licensing

FFmpeg is linked under LGPL v2.1+. See `LICENSE` and FFmpeg's own license
documentation. Decide early whether you need GPL features.

SDL2 is licensed under the zlib license.

miniaudio is dual-licensed under MIT-0 / Public Domain.
