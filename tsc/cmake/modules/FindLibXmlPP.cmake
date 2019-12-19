#############################################################################
# FindLibXmlPP.cmake - CMake module for finding libxml++
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

pkg_check_modules(PKG_LibXmlPP_Glib QUIET glib-2.0)
pkg_check_modules(PKG_LibXmlPP_GlibMM QUIET glibmm-2.4)
pkg_check_modules(PKG_LibXmlPP QUIET libxml++-${LibXmlPP_FIND_VERSION})

# Note we use a subdirectory find (libxml++/document.h) since that is what
# we include. Many other libs may have a document.h file, so one definitely
# does not want that subdirectory into the compiler include search path
# (prevents conflicts).
find_path(LibXmlPP_INCLUDE_DIR libxml++/document.h
  HINTS ${PKG_LibXmlPP_INCLUDE_DIRS} /usr/include/libxml++-${LibXmlPP_FIND_VERSION})

# libxml++'s headers include glibmm headers, which is irritating.
# When including libxml++'s headers, these need to be in the compiler
# search path.
find_path(LibXmlPP_GlibMM_INCLUDE_DIR glibmm/ustring.h
  HINTS ${PKG_LibXmlPP_GlibMM_INCLUDE_DIRS} /usr/include/glibmm-2.4)

# And the glibmm headers include other glibmm headers which they assume
# in the compiler search path...
find_path(LibXmlPP_GlibMM_Config_INCLUDE_DIR glibmmconfig.h
  HINTS ${PKG_LibXmlPP_GlibMM_INCLUDE_DIRS} /usr/lib/glibmm-2.4/include)

# And the glibmm headers also include the glib headers.
find_path(LibXmlPP_Glib_INCLUDE_DIR glib.h
  HINTS ${PKG_LibXmlPP_Glib_INCLUDE_DIRS} /usr/include/glib-2.0)

# And the glib headers include the glib config header.
find_path(LibXmlPP_Glib_Config_INCLUDE_DIR glibconfig.h
  HINTS ${PKG_LibXmlPP_Glib_INCLUDE_DIRS} /usr/lib/glib-2.0/include)

# libxml++'s headers also include a config header.
find_path(LibXmlPP_Config_INCLUDE_DIR libxml++config.h
  HINTS ${PKG_LibXmlPP_INCLUDE_DIRS} /usr/lib/libxml++-${LibXmlPP_FIND_VERSION}/include)

# Now for the actual libraries.
find_library(LibXmlPP_LIBRARY
  NAMES xml++ xml++-${LibXmlPP_FIND_VERSION}
  HINTS ${PKG_LibXmlPP_LIBRARY_DIRS})
find_library(LibXmlPP_GlibMM_LIBRARY
  NAMES glibmm-2.4 glibmm
  HINTS ${PKG_LibXmlPP_GlibMM_LIBRARY_DIRS})
find_library(LibXmlPP_Glib_LIBRARY
  NAMES glib-2.0 glib
  HINTS ${PKG_LibXmlPP_Glib_LIBRARY_DIRS})

set(LibXmlPP_LIBRARIES
  ${LibXmlPP_LIBRARY}
  ${LibXmlPP_GlibMM_LIBRARY}
  ${LibXmlPP_Glib_LIBRARY})
set(LibXmlPP_INCLUDE_DIRS
  ${LibXmlPP_INCLUDE_DIR}
  ${LibXmlPP_GlibMM_INCLUDE_DIR}
  ${LibXmlPP_GlibMM_Config_INCLUDE_DIR}
  ${LibXmlPP_Glib_INCLUDE_DIR}
  ${LibXmlPP_Glib_Config_INCLUDE_DIR}
  ${LibXmlPP_Config_INCLUDE_DIR})

if (PKG_LibXmlPP_CFLAGS)
  set(LibXmlPP_DEFINITIONS ${PKG_LibXmlPP_CFLAGS})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibXmlPP
  REQUIRED_VARS LibXmlPP_LIBRARY LibXmlPP_GlibMM_LIBRARY LibXmlPP_Glib_LIBRARY LibXmlPP_INCLUDE_DIR
  VERSION_VAR PKG_LibXmlPP_VERSION)

mark_as_advanced(LibXmlPP_LIBRARIES LibXmlPP_INCLUDE_DIRS LibXmlPP_DEFINITIONS)

if(LibXmlPP_FOUND)
  if(NOT TARGET LibXmlPP::Glib)
    add_library(LibXmlPP::Glib UNKNOWN IMPORTED)
    set_target_properties(LibXmlPP::Glib PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES
                            "${LibXmlPP_Glib_INCLUDE_DIR};${LibXmlPP_Glib_Config_INCLUDE_DIR}"
                          IMPORTED_LOCATION "${LibXmlPP_Glib_LIBRARY}"
                          IMPORTED_LINK_INTERFACE_LANGUAGES "C")
  endif()

  if(NOT TARGET LibXmlPP::GlibMM)
    add_library(LibXmlPP::GlibMM UNKNOWN IMPORTED)
    set_target_properties(LibXmlPP::GlibMM PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES
                            "${LibXmlPP_GlibMM_INCLUDE_DIR};${LibXmlPP_GlibMM_Config_INCLUDE_DIR}"
                          INTERFACE_LINK_LIBRARIES LibXmlPP::Glib
                          IMPORTED_LOCATION "${LibXmlPP_GlibMM_LIBRARY}"
                          IMPORTED_LINK_INTERFACE_LANGUAGES "C++")
  endif()

  if(NOT TARGET LibXmlPP::LibXmlPP)
    add_library(LibXmlPP::LibXmlPP UNKNOWN IMPORTED)
    set_target_properties(LibXmlPP::LibXmlPP PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES
                            "${LibXmlPP_INCLUDE_DIR};${LibXmlPP_Config_INCLUDE_DIR}"
                          INTERFACE_COMPILE_OPTIONS "${LibXmlPP_DEFINITIONS}"
                          INTERFACE_LINK_LIBRARIES LibXmlPP::GlibMM
                          IMPORTED_LOCATION "${LibXmlPP_LIBRARY}"
                          IMPORTED_LINK_INTERFACE_LANGUAGES "C++")
  endif()
endif()
