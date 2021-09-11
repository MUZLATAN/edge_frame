# This file is used for cross compilation on hisi chip platform

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# set(CMAKE_SYSROOT
# /home/whale/Documents/toolchain_hisi/arm-himix200-linux/target)
# set(CMAKE_STAGING_PREFIX
# /home/whale/Documents/toolchain_hisi/arm-himix200-linux/target/usr/)

set(tools /usr/local/cross-compile/aarch64-cc/)
set(CMAKE_C_COMPILER ${tools}/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
