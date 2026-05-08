find_package(SDL2 REQUIRED)

add_library(media_sdl STATIC
  src/sdl_video_output.cpp
  src/sdl_window.cpp
)

target_include_directories(media_sdl
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(media_sdl
  PUBLIC
    media_core
    SDL2::SDL2
)

if(MSVC)
  target_compile_options(media_sdl PRIVATE /W4 /permissive-)
else()
  target_compile_options(media_sdl PRIVATE -Wall -Wextra -Wpedantic)
endif()
