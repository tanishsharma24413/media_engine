add_executable(libtanish WIN32
  src/main.cpp
)

target_link_libraries(
  libtanish
  PRIVATE
    media_core
    media_win
    media_audio
    user32
    gdi32
    shell32
)

if(MSVC)
  target_compile_options(libtanish PRIVATE /W4 /permissive-)
else()
  target_compile_options(libtanish PRIVATE -Wall -Wextra -Wpedantic)
endif()
