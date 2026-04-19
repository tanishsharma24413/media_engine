cmake_minimum_required(VERSION 3.24)
project(media_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEDIA_ENABLE_FFMPEG "Build FFmpeg-backed demux/decode module" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(core)

if(WIN32)
  add_subdirectory(modules/win)
  add_subdirectory(app)
else()
  message(STATUS "Skipping Win32 prototype (modules/win + app) on non-Windows hosts.")
endif()

if(MEDIA_ENABLE_FFMPEG)
  add_subdirectory(modules/ffmpeg)
endif()
