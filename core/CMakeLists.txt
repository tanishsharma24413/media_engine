add_library(media_core STATIC
  src/player.cpp
  src/engine.cpp
  src/clock.cpp
  src/pipeline.cpp
  src/libtanish.cpp
)

target_include_directories(
  media_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MSVC)
  target_compile_options(media_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(media_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
