# Portable SDL2 find-script for Tanish Player.
#
# Produces the imported target SDL2::SDL2 (and SDL2::SDL2main on Windows).
#
# Windows:  Expects vcpkg or an SDL2_DIR pointing at the SDL2 cmake config.
# Linux / Raspberry Pi: Uses pkg-config (libsdl2-dev from apt).

if(WIN32)
  # vcpkg or manually installed SDL2 config package
  if(NOT SDL2_DIR AND EXISTS "C:/SDL2/cmake")
    set(SDL2_DIR "C:/SDL2/cmake")
  endif()
  find_package(SDL2 CONFIG REQUIRED)
  # vcpkg provides SDL2::SDL2 and SDL2::SDL2main already.
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2_PKG REQUIRED sdl2)

  if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_PKG_INCLUDE_DIRS})
    target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_PKG_LIBRARIES})
    target_compile_options(SDL2::SDL2 INTERFACE ${SDL2_PKG_CFLAGS_OTHER})
  endif()

  if(NOT TARGET SDL2::SDL2main)
    # On Linux the SDL2main stub is embedded in the SDL2 library itself.
    add_library(SDL2::SDL2main ALIAS SDL2::SDL2)
  endif()
endif()
