cmake_minimum_required(VERSION 3.24)
project(libtanish LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEDIA_ENABLE_FFMPEG "Build FFmpeg-backed demux/decode module" ON)
option(MEDIA_ENABLE_SDL    "Build SDL2 video/window module (all platforms)" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Core engine (always built) ────────────────────────────────────────────────
add_subdirectory(core)

# ── Audio (miniaudio, always built) ──────────────────────────────────────────
add_subdirectory(modules/audio)

# ── FFmpeg demux/decode module ────────────────────────────────────────────────
if(MEDIA_ENABLE_FFMPEG)
  add_subdirectory(modules/ffmpeg)
endif()

# ── SDL2 video/window module (universal) ─────────────────────────────────────
if(MEDIA_ENABLE_SDL)
  add_subdirectory(modules/sdl)
endif()

# ── Win32-specific D3D11 presenter (optional high-performance path) ───────────
if(WIN32)
  add_subdirectory(modules/win)
endif()

# ── Application ───────────────────────────────────────────────────────────────
add_subdirectory(app)

# ── Release Packaging (CPack) ─────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "TanishPlayer")
set(CPACK_PACKAGE_VENDOR "Tanish Sharma")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance cross-platform media player")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS") # ZIP for portable, NSIS for installer
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_NSIS_DISPLAY_NAME "Tanish Player")
  set(CPACK_NSIS_PACKAGE_NAME "Tanish Player")
endif()

include(CPack)
