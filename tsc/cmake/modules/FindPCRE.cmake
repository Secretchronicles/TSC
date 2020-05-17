#############################################################################
# FindPCRE.cmake - CMake module for finding libpcre
#
# Copyright © 2012-2020 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

pkg_check_modules(PKG_PCRE QUIET libpcre)

find_path(PCRE_INCLUDE_DIR pcre.h
  HINTS ${PKG_PCRE_INCLUDE_DIRS})
find_library(PCRE_LIBRARY pcre
  HINTS ${PKG_PCRE_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_INCLUDE_DIR PCRE_LIBRARY)

set(PCRE_LIBRARIES ${PCRE_LIBRARY})
set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})

if (PKG_PCRE_CFLAGS)
  set(PCRE_DEFINITIONS ${PKG_PCRE_CFLAGS})
endif()

mark_as_advanced(PCRE_LIBRARIES PCRE_INCLUDE_DIRS PCRE_DEFINITIONS)
