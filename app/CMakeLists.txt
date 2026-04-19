add_executable(media_player WIN32
  src/main.cpp
)

target_link_libraries(
  media_player
  PRIVATE
    media_core
    media_win
    user32
    gdi32
    shell32
)

if(MSVC)
  target_compile_options(media_player PRIVATE /W4 /permissive-)
else()
  target_compile_options(media_player PRIVATE -Wall -Wextra -Wpedantic)
endif()
