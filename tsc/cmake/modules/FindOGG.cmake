#############################################################################
# FindOGG.cmake - CMake module for finding the OGG audio decoder
#
# Copyright © 2012-2017 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

pkg_check_modules(PKG_OGG QUIET ogg)

find_path(OGG_INCLUDE_DIR ogg.h
  HINTS ${PKG_OGG_INCLUDE_DIRS}
  PATH_SUFFIXES ogg)
find_library(OGG_LIBRARY
  NAMES ogg OGG
  HINTS ${PKG_OGG_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OGG DEFAULT_MSG OGG_INCLUDE_DIR OGG_LIBRARY)

set(OGG_LIBRARIES ${OGG_LIBRARY})
set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})

if (PKG_OGG_CFLAGS)
  set(OGG_DEFINITIONS ${PKG_OGG_CFLAGS})
endif()

mark_as_advanced(OGG_LIBRARIES OGG_INCLUDE_DIRS OGG_DEFINITIONS)

if(OGG_FOUND AND NOT TARGET OGG::OGG)
  add_library(OGG::OGG UNKNOWN IMPORTED)
  set_target_properties(OGG::OGG PROPERTIES
                        INTERFACE_COMPILE_OPTIONS "${OGG_DEFINITIONS}"
                        INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${OGG_LIBRARY}"
                        IMPORTED_LINK_INTERFACE_LANGUAGES "C")
endif()
