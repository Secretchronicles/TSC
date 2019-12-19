#############################################################################
# FindRuby.cmake - CMake module for finding Ruby
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
##
# This file exists because CMake's provided FindRuby.cmake wants to
# find the ruby embedding library, which is not what we want here
# at all. We only want the Ruby interpreter for our build scripts.
# Note: This has nothing to do with mruby!!

find_program(RUBY_EXECUTABLE ruby)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ruby
  DEFAULT_MSG
  RUBY_EXECUTABLE)
