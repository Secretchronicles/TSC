#############################################################################
# FindVorbis.cmake - CMake module for finding the Vorbis audio decoder
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

pkg_check_modules(PKG_Vorbis QUIET vorbis)

find_path(Vorbis_INCLUDE_DIR vorbis/codec.h
  HINTS ${PKG_Vorbis_INCLUDE_DIRS})
find_library(Vorbis_LIBRARY vorbis
  HINTS ${PKG_Vorbis_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis DEFAULT_MSG Vorbis_INCLUDE_DIR Vorbis_LIBRARY)

set(Vorbis_LIBRARIES ${Vorbis_LIBRARY})
set(Vorbis_INCLUDE_DIRS ${Vorbis_INCLUDE_DIR})

if (PKG_Vorbis_CFLAGS)
  set(Vorbis_DEFINITIONS ${PKG_Vorbis_CFLAGS})
endif()

mark_as_advanced(Vorbis_LIBRARIES Vorbis_INCLUDE_DIRS Vorbis_DEFINITIONS)

if(Vorbis_FOUND AND NOT TARGET Vorbis::Vorbis)
  add_library(Vorbis::Vorbis UNKNOWN IMPORTED)
  set_target_properties(Vorbis::Vorbis PROPERTIES
                        INTERFACE_COMPILE_OPTIONS "${Vorbis_DEFINITIONS}"
                        INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${Vorbis_LIBRARY}"
                        IMPORTED_LINK_INTERFACE_LANGUAGES "C")
endif()
