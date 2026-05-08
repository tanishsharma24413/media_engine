# Helpers when MEDIA_ENABLE_FFMPEG is ON.
# Expect FFMPEG_ROOT to point at a dev prefix with include/ and lib/.

function(media_apply_ffmpeg target_name)
  if(NOT MEDIA_ENABLE_FFMPEG)
    message(FATAL_ERROR "media_apply_ffmpeg called with MEDIA_ENABLE_FFMPEG=OFF")
  endif()

  if(WIN32)
    set(_root "${FFMPEG_ROOT}")
    if(NOT _root AND DEFINED ENV{FFMPEG_ROOT})
      set(_root "$ENV{FFMPEG_ROOT}")
    endif()

    if(NOT _root AND EXISTS "C:/ffmpeg")
      set(_root "C:/ffmpeg")
    endif()

    if(NOT _root)
      message(FATAL_ERROR 
        "FFmpeg development prefix not found!\n"
        "Please set FFMPEG_ROOT to your FFmpeg prefix (contains include/, lib/).\n"
        "Download pre-built dev binaries here: https://github.com/BtbN/FFmpeg-Builds/releases\n"
        "Suggested path: C:/ffmpeg"
      )
    endif()

    target_include_directories(${target_name} PRIVATE "${_root}/include")
    target_link_directories(${target_name} PRIVATE "${_root}/lib")

    target_link_libraries(
      ${target_name}
      PRIVATE
        avformat
        avcodec
        avutil
        swscale
        swresample
        avfilter
        avdevice
    )
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample libavfilter libavdevice)
    target_include_directories(${target_name} PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE ${FFMPEG_LIBRARIES})
  endif()

  target_compile_definitions(${target_name} PRIVATE MEDIA_WITH_FFMPEG=1)
endfunction()
