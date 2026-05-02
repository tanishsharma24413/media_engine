cmake_minimum_required(VERSION 3.24)
project(libtanish LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEDIA_ENABLE_FFMPEG "Build FFmpeg-backed demux/decode module" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(core)
add_subdirectory(modules/audio)

if(WIN32)
  add_subdirectory(modules/win)
  add_subdirectory(app)
else()
  # Linux / ARM support
  message(STATUS "Configuring for Linux/Unix. Win32 app skipped.")
  # TODO: Add modules/sdl or modules/x11 for Linux rendering.
endif()

if(MEDIA_ENABLE_FFMPEG)
  add_subdirectory(modules/ffmpeg)
endif()
