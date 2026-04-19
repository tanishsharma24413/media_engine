set(FFMPEG_ROOT "" CACHE PATH "FFmpeg prefix (contains include/, lib/)")

include(MediaFFmpeg)

add_library(media_ffmpeg STATIC
  src/ffmpeg_backend.cpp
)

target_include_directories(
  media_ffmpeg
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(media_ffmpeg PUBLIC media_core)
media_apply_ffmpeg(media_ffmpeg)

if(MSVC)
  target_compile_options(media_ffmpeg PRIVATE /W4 /permissive-)
else()
  target_compile_options(media_ffmpeg PRIVATE -Wall -Wextra -Wpedantic)
endif()
