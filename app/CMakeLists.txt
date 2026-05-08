set(PLAYER_SOURCES src/main.cpp)

# On Windows we embed a resource file for the icon + version info and use the
# WIN32 subsystem so no console window appears (SDL2main handles WinMain→main).
if(WIN32)
  list(APPEND PLAYER_SOURCES res/tanish_player.rc)
  add_executable(TanishPlayer WIN32 ${PLAYER_SOURCES})
else()
  add_executable(TanishPlayer ${PLAYER_SOURCES})
endif()

find_package(SDL2 REQUIRED)

target_link_libraries(TanishPlayer
  PRIVATE
    media_core
    media_sdl
    media_audio
    media_ffmpeg
    SDL2::SDL2
)

# SDL2main provides the WinMain stub that calls SDL_main(=main) on Windows.
if(WIN32)
  target_link_libraries(TanishPlayer PRIVATE SDL2::SDL2main comdlg32 shell32)
endif()

# SDL_ttf is optional — detected here and propagated as a compile definition.
find_package(PkgConfig QUIET)
if(WIN32)
  find_package(SDL2_ttf CONFIG QUIET)
  if(TARGET SDL2_ttf::SDL2_ttf)
    target_link_libraries(TanishPlayer PRIVATE SDL2_ttf::SDL2_ttf)
    target_compile_definitions(TanishPlayer PRIVATE TANISH_HAS_SDL_TTF=1)
  endif()
elseif(PkgConfig_FOUND)
  pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
  if(SDL2_TTF_FOUND)
    target_include_directories(TanishPlayer PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
    target_link_libraries(TanishPlayer PRIVATE ${SDL2_TTF_LIBRARIES})
    target_compile_definitions(TanishPlayer PRIVATE TANISH_HAS_SDL_TTF=1)
  endif()
endif()

if(MSVC)
  target_compile_options(TanishPlayer PRIVATE /W4 /permissive-)
else()
  target_compile_options(TanishPlayer PRIVATE -Wall -Wextra -Wpedantic)
endif()
