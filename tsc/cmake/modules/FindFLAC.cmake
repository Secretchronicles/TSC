#############################################################################
# FindFLAC.cmake - CMake module for finding the FLAC audio decoder
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

pkg_check_modules(PKG_FLAC QUIET flac)

find_path(FLAC_INCLUDE_DIR FLAC/all.h
  HINTS ${PKG_FLAC_INCLUDE_DIRS})
find_library(FLAC_LIBRARY
  NAMES flac FLAC
  HINTS ${PKG_FLAC_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC DEFAULT_MSG FLAC_INCLUDE_DIR FLAC_LIBRARY)

set(FLAC_LIBRARIES ${FLAC_LIBRARY})
set(FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR})

if (PKG_FLAC_CFLAGS)
  set(FLAC_DEFINITIONS ${PKG_FLAC_CFLAGS})
endif()

mark_as_advanced(FLAC_LIBRARIES FLAC_INCLUDE_DIRS FLAC_DEFINITIONS)

if(FLAC_FOUND AND NOT TARGET FLAC::FLAC)
  add_library(FLAC::FLAC UNKNOWN IMPORTED)
  set_target_properties(FLAC::FLAC PROPERTIES
                        INTERFACE_COMPILE_OPTIONS "${FLAC_DEFINITIONS}"
                        INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${FLAC_LIBRARY}"
                        IMPORTED_LINK_INTERFACE_LANGUAGES "C")
endif()
