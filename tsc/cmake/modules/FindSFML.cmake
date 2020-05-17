################################################################################
# FindSFML.cmake - CMake module for finding SFML
#
# Copyright © 2012-2020 The TSC Contributors
################################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
##
#
# Usage:
#
# find_package(SFML COMPONENTS ... [REQUIRED])
#
# Possible components are: audio, graphics, network, system, window.
# You almost always want the "system" component.
#
# Example:
#
# find_package(SFML COMPONENTS window system REQUIRED)
#
# This module sets the following variables:
#
# SFML_INCLUDE_DIRS: Path to the SFML library headers
# SFML_LIBRARIES: List of all requested component libraries
# SFML_DEFINITIONS: Possible required cflags such as SFML_STATIC
#
# Also sets SFML_INCLUDE_DIR to the same value as SFML_INCLUDE_DIRS,
# for compatibility.

pkg_search_module(SFMLPKG QUIET sfml-all sfml)

find_path(SFML_INCLUDE_DIRS Graphics.hpp
  HINTS ${SFMLPKG_INCLUDE_DIRS}
  PATH_SUFFIXES sfml SFML)

set(SFML_LIBRARIES "")

foreach(component ${SFML_FIND_COMPONENTS})
  message(STATUS "Searching for SFML ${component} library")

  find_library(SFML_${component}_LIBRARY
    NAMES sfml-${component} sfml-${component}-s
    HINTS ${SFMLPKG_LIBRARY_DIRS}
    PATH_SUFFIXES sfml SFML)

  if (SFML_${component}_LIBRARY)
    list(APPEND SFML_LIBRARIES ${SFML_${component}_LIBRARY})
    message(STATUS "  found: ${SFML_${component}_LIBRARY}")
  else()
    message(SEND_ERROR "  SFML ${component} library not found!")
  endif()
endforeach()

set(SFML_INCLUDE_DIR ${SFML_INCLUDE_DIRS})
set(SFML_DEFINITIONS ${SFMLPKG_CFLAGS})
mark_as_advanced(SFML_LIBRARIES SFML_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SFML DEFAULT_MSG SFML_INCLUDE_DIRS SFML_LIBRARIES)

