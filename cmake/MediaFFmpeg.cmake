# Helpers when MEDIA_ENABLE_FFMPEG is ON.
# Expect FFMPEG_ROOT to point at a dev prefix with include/ and lib/.

function(media_apply_ffmpeg target_name)
  if(NOT MEDIA_ENABLE_FFMPEG)
    message(FATAL_ERROR "media_apply_ffmpeg called with MEDIA_ENABLE_FFMPEG=OFF")
  endif()

  if(WIN32)
    set(_root "${FFMPEG_ROOT}")
    if(NOT _root)
      message(FATAL_ERROR "Set FFMPEG_ROOT to your FFmpeg prefix (contains include/, lib/).")
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
    )
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libswresample)
    target_include_directories(${target_name} PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE ${FFMPEG_LIBRARIES})
  endif()

  target_compile_definitions(${target_name} PRIVATE MEDIA_WITH_FFMPEG=1)
endfunction()
