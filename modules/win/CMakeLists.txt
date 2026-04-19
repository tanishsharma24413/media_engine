add_library(media_win STATIC
  src/d3d11_presenter.cpp
)

target_include_directories(
  media_win
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
  media_win
  PUBLIC
    media_core
  PRIVATE
    d3d11
    dxgi
)

if(MSVC)
  target_compile_options(media_win PRIVATE /W4 /permissive-)
else()
  target_compile_options(media_win PRIVATE -Wall -Wextra -Wpedantic)
endif()
