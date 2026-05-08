add_library(media_audio STATIC
  src/miniaudio_output.cpp
)

target_include_directories(
  media_audio
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
  media_audio
  PUBLIC
    media_core
)

if(MSVC)
  target_compile_options(media_audio PRIVATE /W0) # miniaudio has warnings
else()
  target_compile_options(media_audio PRIVATE -w)
endif()
