#############################################################################
# FindFreeImage.cmake - CMake module for finding FreeImage
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

find_path(FreeImage_INCLUDE_DIR FreeImage.h)
find_library(FreeImage_LIBRARY freeimage)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage DEFAULT_MSG FreeImage_INCLUDE_DIR FreeImage_LIBRARY)
set(FreeImage_LIBRARIES ${FreeImage_LIBRARY})
set(FreeImage_INCLUDE_DIRS ${FreeImage_INCLUDE_DIR})
