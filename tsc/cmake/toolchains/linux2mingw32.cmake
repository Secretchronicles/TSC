#############################################################################
# linux2win32.cmake - CMake toolchain file for MinGW crosscompilation
#
# Copyright © 2013-2014 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# CMake toolchain definition file for cross-compiling
# TSC from a Linux host to the win32 target, using the
# Minimalist GNU for Windows (MinGW) system.
set(CMAKE_SYSTEM_NAME Windows)

########################################
# CUSTOMIZE SECTION
########################################

# Replace this with the actual host triplet of
# your cross-compilation toolchain. The compiler
# name etc. will automatically be appended to this.
set(TOOLCHAIN_PREFIX i686-pc-mingw32)

# Replace this with the root path(s) of your cross-compilation
# environment. I recommend MXE (http://mxe.cc) for setting up one.
# $ENV{HOME} gets replaced with the absolute path to your home directory.
# Examples:
#   set(CMAKE_FIND_ROOT_PATH "/opt/crossenv")
#   set(CMAKE_FIND_ROOT_PATH "$ENV{HOME}/crossenv" "$ENV{HOME}/otherstuffhere")
#   set(CMAKE_FIND_ROOT_PATH "/opt/crossenvs/i686-mingw32")
set(CMAKE_FIND_ROOT_PATH "$ENV{HOME}/mxe/usr/i686-pc-mingw32")

########################################
# END OF CUSTOMIZE SECTION
########################################

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
