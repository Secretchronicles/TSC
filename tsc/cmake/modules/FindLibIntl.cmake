#############################################################################
# FindLibIntl.cmake - CMake module for finding LibIntl
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

find_path(LIBINTL_INCLUDE_DIR libintl.h)
find_library(LIBINTL_LIBRARY NAMES intl libintl)
set(LIBINTL_LIBRARIES ${LIBINTL_LIBRARY})
set(LIBINTL_INCLUDE_DIRS ${LIBINTL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibIntl
  DEFAULT_MSG
  LIBINTL_LIBRARY
  LIBINTL_INCLUDE_DIR)

mark_as_advanced(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARY)
