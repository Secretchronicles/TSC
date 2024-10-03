# Try to find the CEGUI library.
# Only works for CEGUI >= 0.8.0.
# The following variables will be defined:
#
# CEGUI_FOUND - Do we have CEGUI?
# CEGUI_INCLUDE_DIR - Headers for CEGUI
# CEGUI_LIBRARIES - Libraries for CEGUI
# CEGUI_DEFINITIONS - C/CXX flags
#
# Usage:
#
#   FindPackage(CEGUI COMPONENTS OpenGL)
#
# The COMPONENTS part defines the CEGUI renderer to use; the example
# above will find the OpenGL renderer.
#
# Copyright © 2014, 2016 Marvin Gülker

# CEGUI 0.8.7 does not build against Debian 10's libxml2,
# so provide the possibility to use expat instead.
# Do not apply this workaround option unless you really
# need it.
option(CEGUI_USE_EXPAT "Workaround option: advise CEGUI to use expat instead of libxml2 -- this option has nothing to do with TSC's own use of libxml. This option is ignored if USE_SYSTEM_CEGUI is set to OFF." OFF)

########################################
# pkg-config

# First we need pkg-config.
find_package(PkgConfig)

# Check if we can find it with pkg-config.
# (No, I don’t know why they decided to rename
# themselves to CEGUI-0, really!)
pkg_search_module(PKG_CEGUI QUIET CEGUI CEGUI-0)

########################################
# Include dir

# CEGUI’s pkg-config file is broken -- it says /usr/include/cegui-0/cegui,
# but the real include dir is /usr/include/cegui-0/CEGUI. I hate Windows
# devs who don’t care about casefolding. Also the appended "-0" CEGUI
# uses causes problems for CMake, so we give additional PATH_SUFFIXES
# to try.
find_path( CEGUI_INCLUDE_DIR CEGUI/CEGUI.h
  HINTS ${PKG_CEGUI_INCLUDE_DIRS} /usr/include/cegui-0/CEGUI
  PATH_SUFFIXES cegui-0 CEGUI)

########################################
# The libraries

# We need to find a lot of libraries, so this macro
# does the main work here.
macro(find_cegui_library LIBNAME)
  message("-- Searching for ${LIBNAME} CEGUI library")

  # CEGUI 0.8 scattered its libraries over /usr/lib and
  # /usr/lib/cegui-0.8, AND it sometimes has a "-0" appended
  # to the library name.
  find_library( CEGUI_${LIBNAME}_LIBRARY
                NAMES CEGUI${LIBNAME} CEGUI${LIBNAME}-0 CEGUI${LIBNAME}_Static CEGUI${LIBNAME}-0_Static
                HINTS ${PKG_CEGUI_LIBRARY_DIRS}
                PATH_SUFFIXES cegui-0.8 )

  # Error message if not found
  if(CEGUI_${LIBNAME}_LIBRARY)
    message("--   found: ${CEGUI_${LIBNAME}_LIBRARY}")
  else()
    message(SEND_ERROR "CEGUI${LIBNAME} library not found!")
  endif()
endmacro()

# CEGUI consists of a wealth of libraries.
find_cegui_library(Base)
find_cegui_library(CoreWindowRendererSet)
find_cegui_library(DevILImageCodec)

if (CEGUI_USE_EXPAT)
  message(STATUS "Configuring to link with CEGUI's expat parser instead of libxml2")
  find_cegui_library(ExpatParser)
else()
  find_cegui_library(LibXMLParser)
endif()

########################################
# The renderers

# User is required to selected a renderer.
# (CEGUI_FIND_COMPONENTS is set by package_handle_standard_args())
if( "${CEGUI_FIND_COMPONENTS}" STREQUAL "" )
  message(SEND_ERROR(" No renderer selected; try ...COMPENENTS OpenGL"))
endif()

set(CEGUI_RENDERER_LIBRARIES "")
foreach(COMPONENT ${CEGUI_FIND_COMPONENTS})
  find_cegui_library("${COMPONENT}Renderer")
  list(APPEND CEGUI_RENDERER_LIBRARIES ${CEGUI_${COMPONENT}Renderer_LIBRARY})
endforeach(COMPONENT)

# Now collect all the libraries in a single variable
set(CEGUI_LIBRARIES
  ${CEGUI_RENDERER_LIBRARIES}
  ${CEGUI_Base_LIBRARY}
  ${CEGUI_CoreWindowRendererSet_LIBRARY}
  ${CEGUI_DevILImageCodec_LIBRARY}
 )

if(CEGUI_USE_EXPAT)
  list(APPEND CEGUI_LIBRARIES ${CEGUI_ExpatParser_LIBRARY})
else()
  list(APPEND CEGUI_LIBRARIES ${CEGUI_LibXMLParser_LIBRARY})
endif()

########################################
# Flags

# This sets -DCEGUI_STATIC! If you forget including
# that into your compilation commands, you will get
# wealths of "undefined references to _imp__*" errors
# on crosscompilation!
set(CEGUI_DEFINITIONS ${PKG_CEGUI_CFLAGS})

########################################
# Package boilerplate

# Register as a CMake module. This sets CEGUI_FIND_COMPONENTS
# to the list supplied by the calling user after COMPONENTS.
find_package_handle_standard_args(CEGUI
  DEFAULT_MSG
  CEGUI_INCLUDE_DIR
  CEGUI_LIBRARIES)

# These variables may be set by the configuring user if
# he knows what he is doing.
mark_as_advanced(CEGUI_INCLUDE_DIR
  CEGUI_BASE_LIBRARY
  CEGUI_CORE_WINDOW_RENDERER_SET_LIBRARY
  CEGUI_DEVIL_CODEC_LIBRARY
  CEGUI_LIBXML_PARSER_LIBRARY
  CEGUI_EXPAT_PARSER_LIBRARY
  CEGUI_RENDERER_LIBRARIES
  CEGUI_LIBRARIES)
