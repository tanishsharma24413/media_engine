# libtanish (desktop prototype)

This is a **Windows-first scaffold** for a proprietary media core:

- `core/`: `media::Player` API + engine thread + pipeline placeholders + clock stub
- `modules/win/`: minimal **D3D11** presenter (`IVideoOutput`)
- `app/`: **Win32** shell (no Qt) with a simple render loop

FFmpeg is optional and isolated under `modules/ffmpeg/` (off by default).

## Build (Windows, MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Run:

```powershell
.\build\app\Release\media_player.exe "C:\path\to\file.mp4"
```

Notes:

- The window clears a dark color at ~1000 FPS (with `Sleep(1)`), enough to validate D3D11 presentation. Replace with a paced vsync loop when decoding is wired in.
- `Space` toggles `play`/`pause`, `Esc` calls `stop` (state machine only; no decode yet).

## Optional FFmpeg module

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMEDIA_ENABLE_FFMPEG=ON -DFFMPEG_ROOT="C:/path/to/ffmpeg/prefix"
cmake --build build --config Release
```

`FFMPEG_ROOT` must contain `include/` and `lib/` with `avformat`, `avcodec`, `avutil`, `swscale`, `swresample`.

Licensing note: how you link FFmpeg matters (LGPL vs GPL features). Decide early with legal counsel.
