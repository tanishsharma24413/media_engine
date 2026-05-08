# Raspberry Pi 4/5 cross-compile toolchain (AArch64 Linux)
#
# Prerequisites on the host (Ubuntu/Debian x64):
#   sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
#                    libsdl2-dev:arm64 libavformat-dev:arm64 \
#                    libavcodec-dev:arm64 libavutil-dev:arm64 \
#                    libswscale-dev:arm64 libswresample-dev:arm64
#
# Usage:
#   cmake -B build-rpi -S . \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/rpi-aarch64.cmake \
#     -DMEDIA_ENABLE_FFMPEG=ON

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT /usr/aarch64-linux-gnu)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
