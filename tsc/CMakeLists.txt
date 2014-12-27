#############################################################################
# CMakeLists.txt  -  Build file
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

cmake_minimum_required(VERSION 2.8)
enable_language(CXX)
project(TSC)

# Ensure our custom CMake modules get found
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Some CMake modules we use
include(ExternalProject)

########################################
# Version number

# If you get a merge conflict below, always keep "dev" on the
# "devel" branch. The postfix is only to be altered on release
# branches.
set(TSC_VERSION_MAJOR 2)
set(TSC_VERSION_MINOR 1)
set(TSC_VERSION_PATCH 0)
set(TSC_VERSION_POSTFIX "dev")

###############################################
# Flags & Options

option(ENABLE_MRUBY "Enable the MRuby scripting engine" ON)
option(ENABLE_NLS "Enable translations and localisations" ON)
set(FIXED_DATA_DIR "" CACHE FILEPATH "Enforce a static directory to look up graphics, music, etc. under rather than having TSC determine it dynamically.")

#FIXME: Actually use ENABLE_MRUBY

# Verbose compilation for better code quality
if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions("-Wall")
endif()

# Extra flags for debugging TSC, additional to those
# already added by CMake itself when doing a debug
# build (e.g. -g is added for GCC by CMake).
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG=1")
endif()

execute_process(COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${TSC_SOURCE_DIR}
  OUTPUT_VARIABLE TSC_VERSION_GIT
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND nproc
  OUTPUT_VARIABLE NUM_CPUCORES
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# Configuration options for the library finding
# functions.
set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_STATIC_RUNTIME OFF)

########################################
# Cross-compilation

if (TOOLCHAIN_PREFIX)
  message(STATUS "Cross-compilation detected: ${TOOLCHAIN_PREFIX}")
endif()

################################################################################
# EXTERNAL PROJECTS
################################################################################

###############################################
# mruby

if (ENABLE_MRUBY)
  # Rake, for building mruby
  find_program(RAKE_EXECUTABLE rake)

  # Tell CMake how to compile mruby
  set(MRUBY_TOPDIR ${TSC_SOURCE_DIR}/../mruby)
  set(MRUBY_ROOT_DIR ${MRUBY_TOPDIR}/mruby)
  set(MRUBY_MGEMS_DIR ${MRUBY_TOPDIR}/mgems)

  ExternalProject_Add(
    mruby
    PREFIX "${TSC_BINARY_DIR}/mruby"
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
    SOURCE_DIR "${MRUBY_ROOT_DIR}"
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${RAKE_EXECUTABLE} MRUBY_CONFIG=${TSC_SOURCE_DIR}/mruby_tsc_build_config.rb CROSSCOMPILE_TARGET=${TOOLCHAIN_PREFIX}
    INSTALL_COMMAND "")

  set(MRuby_INCLUDE_DIR ${MRUBY_ROOT_DIR}/include)

  if(TOOLCHAIN_PREFIX)
    set(MRuby_LIBRARIES "${MRUBY_ROOT_DIR}/build/${TOOLCHAIN_PREFIX}/lib/libmruby.a" "${MRUBY_ROOT_DIR}/build/${TOOLCHAIN_PREFIX}/lib/libmruby_core.a")
  else()
    set(MRuby_LIBRARIES "${MRUBY_ROOT_DIR}/build/host/lib/libmruby.a" "${MRUBY_ROOT_DIR}/build/host/lib/libmruby_core.a")
  endif()
else() # Disable it
  set(MRuby_LIBRARIES "")
endif()

########################################
# CEGUI 0.7.x
# Many Linux distributions do not provide CEGUI 0.7.x anymore.
# Hence, if we do not detect it, try to download and compile it
# ourselves.

if (NOT(WIN32))
  find_package(CEGUI COMPONENTS OPENGL)

  if (NOT(CEGUI_FOUND))
    message("CEGUI 0.7.x was not found. It will be downloaded and compiled during the build process.")

    set(CEGUI_ROOT_DIR "${TSC_BINARY_DIR}/cegui")
    set(CEGUI_SOURCE_DIR "${CEGUI_ROOT_DIR}/src/cegui07")

    ExternalProject_Add(cegui07
      PREFIX "${CEGUI_ROOT_DIR}"
      URL "http://mirrorservice.org/sites/dl.sourceforge.net/pub/sourceforge/c/cr/crayzedsgui/CEGUI%20Mk-2/0.7.9/CEGUI-0.7.9.tar.gz"
      URL_MD5 a8b682daf82e383edc169c2e6eb2b321
      BUILD_IN_SOURCE 1
      CONFIGURE_COMMAND ./configure --disable-samples --disable-python-module --disable-lua-module --enable-static --disable-shared --prefix=${CEGUI_ROOT_DIR}
      BUILD_COMMAND make -j${NUM_CPUCORES}
      INSTALL_COMMAND make install)

    set(CEGUI_INCLUDE_DIR ${CEGUI_ROOT_DIR}/include)
    set(CEGUI_LIBRARY ${CEGUI_ROOT_DIR}/lib/libCEGUIOpenGLRenderer.a ${CEGUI_ROOT_DIR}/lib/libCEGUIBase.a ${CEGUI_ROOT_DIR}/lib/libCEGUIDevILImageCodec.a ${CEGUI_ROOT_DIR}/lib/libCEGUIFalagardWRBase.a ${CEGUI_ROOT_DIR}/lib/libCEGUILibxmlParser.a)
    set(CEGUI_DEFINITIONS " -DCEGUI_STATIC")
  endif()
endif()

################################################################################
# Check for the actual libraries and programs.
################################################################################

if (ENABLE_MRUBY)
  find_program(GPERF_EXECUTABLE gperf)
  if(RAKE_EXECUTABLE)
    message(STATUS "Found rake: ${RAKE_EXECUTABLE}")
  else()
    message(SEND_ERROR "Cannot find a rake executable")
  endif()
  if(GPERF_EXECUTABLE)
    message(STATUS "Found gperf: ${GPERF_EXECUTABLE}")
  else()
    message(SEND_ERROR "Cannot find a gperf executable")
  endif()
endif()

# Several libraries we depend on
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Gettext)
find_package(BISON REQUIRED)
find_package(Freetype REQUIRED)

if (WIN32)
  find_package(CEGUI COMPONENTS OPENGL REQUIRED) # Old CEGUI 0.7.x is provided by MXE
  find_package(LibIntl REQUIRED)
  find_package(FreeImage REQUIRED)

  # Boost thread lib is named thread_win32 only under crosscompilation
  if (TOOLCHAIN_PREFIX)
    find_package(Boost 1.50.0
      COMPONENTS filesystem chrono thread_win32 system
      REQUIRED)
  else()
    find_package(Boost 1.50.0
      COMPONENTS filesystem chrono thread system
      REQUIRED)
  endif()

  # Boost-thread and CEGUI need to be explicitely told that they’re
  # linked in statically on Windows.
  add_definitions(-DBOOST_THREAD_USE_LIB)
else()
  set(Boost_USE_STATIC_LIBS OFF)
  find_package(DevIL REQUIRED)
  find_package(Boost 1.50.0
    COMPONENTS filesystem chrono thread system
    REQUIRED)
endif()

pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(SDL REQUIRED sdl)
pkg_check_modules(SDL_IMAGE REQUIRED SDL_image)
pkg_check_modules(SDL_MIXER REQUIRED SDL_mixer)
pkg_check_modules(SDL_TTF REQUIRED SDL_ttf)
pkg_check_modules(PCRE REQUIRED libpcre)
pkg_check_modules(LibXmlPP REQUIRED libxml++-2.6)

###############################################
# Definitions etc.

# Add any definitiony required by libraries
add_definitions(
  ${CEGUI_DEFINITIONS}
  ${PNG_DEFINITIONS}
  )

# Add all our libraries to our -I-nclude path
include_directories(
  ${MRuby_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${CEGUI_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIRS}
  ${SDL_INCLUDE_DIRS}
  ${SDL_IMAGE_INCLUDE_DIRS}
  ${SDL_MIXER_INCLUDE_DIRS}
  ${SDL_TTF_INCLUDE_DIRS}
#  ${IL_INCLUDE_DIR}
  ${PCRE_INCLUDE_DIRS}
  ${LibXmlPP_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  )

if (WIN32)
  include_directories(${FreeImage_INCLUDE_DIRS})
else()
  include_directories(${IL_INCLUDE_DIR})
endif()

########################################
# Gettext internationalization

# Process all PO files into MO files usable at runtime.
file(GLOB po_files "data/translations/*.po")
gettext_process_po_files(languages ALL PO_FILES ${po_files})

###############################################
# Configuration header

configure_file(${TSC_SOURCE_DIR}/config.hpp.in
  ${TSC_BINARY_DIR}/config.hpp)
include_directories(${TSC_BINARY_DIR})

########################################
# Credits file

set_source_files_properties(${TSC_BINARY_DIR}/credits.cpp
  PROPERTIES GENERATED true)
add_custom_command(OUTPUT ${TSC_BINARY_DIR}/credits.cpp
  COMMAND rake ${TSC_BINARY_DIR}/credits.cpp
  DEPENDS ${TSC_SOURCE_DIR}/docs/authors.txt
  WORKING_DIRECTORY ${TSC_SOURCE_DIR})

########################################
# Windows icon resource
# See http://stackoverflow.com/a/708382

if(WIN32)
  # First find the windres command
  if(TOOLCHAIN_PREFIX)
    set(WINDRES_COMMAND "${TOOLCHAIN_PREFIX}-windres")
  else()
    set(WINDRES_COMMAND windres)
  endif()

  # Generate the RC file, then compile it.
  configure_file(${TSC_SOURCE_DIR}/icon.rc.in
    ${TSC_BINARY_DIR}/icon.rc)
  add_custom_command(OUTPUT ${TSC_BINARY_DIR}/icon.res
    COMMAND ${WINDRES_COMMAND} ${TSC_BINARY_DIR}/icon.rc -O coff -o ${TSC_BINARY_DIR}/icon.res
    DEPENDS ${TSC_BINARY_DIR}/icon.rc)
endif()

###############################################
# Source files

# For now, just assume ALL cpp files to be requisites
# of TSC.
file(GLOB_RECURSE tsc_sources
  "src/*.cpp"
  "src/*.hpp"
  )

###############################################
# Targets

# Now add our build targets and their dependencies.
if (WIN32)
  add_executable(tsc ${tsc_sources} ${TSC_BINARY_DIR}/credits.cpp icon.res)
else()
  add_executable(tsc ${tsc_sources} ${TSC_BINARY_DIR}/credits.cpp)
endif()

if (ENABLE_MRUBY)
  add_dependencies(tsc mruby)
endif()

# TODO: Make the mruby config automatically include the correct path to the PCRE library
if (WIN32)
  target_link_libraries(tsc
    ${CEGUI_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${FreeImage_LIBRARIES}
    ${SDL_STATIC_LIBRARIES}
    ${SDL_IMAGE_STATIC_LIBRARIES}
    ${SDL_MIXER_STATIC_LIBRARIES}
    ${SDL_TTF_STATIC_LIBRARIES}
    ${Boost_LIBRARIES}
    ${MRuby_LIBRARIES}
    ${PCRE_STATIC_LIBRARIES}
    ${LibXmlPP_STATIC_LIBRARIES}
    ${PNG_STATIC_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    intl
    ws2_32
    ${TSC_BINARY_DIR}/icon.res
    )
  # For some unknown reason, nobody knows how to properly detect libintl?
  # However, as we already require Gettext anyway, linking libintl in directly
  # should be safe.
else()
  add_dependencies(tsc cegui07)
  target_link_libraries(tsc
    ${CEGUI_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${IL_LIBRARIES}
    ${ILU_LIBRARIES}
    ${SDL_LIBRARIES}
    ${SDL_IMAGE_LIBRARIES}
    ${SDL_MIXER_LIBRARIES}
    ${SDL_TTF_LIBRARIES}
    ${Boost_LIBRARIES}
    ${MRuby_LIBRARIES}
    ${PCRE_LIBRARIES}
    ${LibXmlPP_LIBRARIES}
    ${PNG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_DL_LIBS}
    )
endif()

# Installation instructions
install(TARGETS tsc
  DESTINATION bin
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/campaigns/" # Note trailing slash for content copy
  DESTINATION share/tsc/campaigns
  COMPONENT campaigns)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/editor/"
  DESTINATION share/tsc/editor
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/gui/"
  DESTINATION share/tsc/gui
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/icon/"
  DESTINATION share/tsc/icon
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/levels/"
  DESTINATION share/tsc/levels
  COMPONENT levels)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/music/"
  DESTINATION share/tsc/music
  COMPONENT music)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/pixmaps/"
  DESTINATION share/tsc/pixmaps
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/schema/"
  DESTINATION share/tsc/schema
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/scripting/"
  DESTINATION share/tsc/scripting
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/sounds/"
  DESTINATION share/tsc/sounds
  COMPONENT sounds)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/worlds/"
  DESTINATION share/tsc/worlds
  COMPONENT worlds)

foreach(pofile ${po_files})
  get_filename_component(lang ${pofile} NAME_WE)

  install(FILES "${TSC_BINARY_DIR}/${lang}.gmo" # .gmo is generated by gettext_process_po_files() further above
    DESTINATION share/tsc/translations/${lang}/LC_MESSAGES
    RENAME "Secretchronicles.mo" # This must match TSC_GETTEXT_DOMAIN from global_game.hpp so Gettext finds the files
    COMPONENT base)
endforeach()

########################################
# Packager

# Main info
set(CPACK_PACKAGE_NAME "TSC")
set(CPACK_PACKAGE_VENDOR "TSC Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TSC - 2-dimensional platform game")
set(CPACK_PACKAGE_VERSION_MAJOR ${TSC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${TSC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${TSC_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TSC")
set(CPACK_PACKAGE_EXECUTABLES tsc "TSC")

# Nicer names
set(CPACK_COMPONENT_BASE_DISPLAY_NAME "Basic application components (required)")
set(CPACK_COMPONENT_CAMPAIGNS_DISPLAY_NAME "Core campaigns")
set(CPACK_COMPONENT_LEVELS_DISPLAY_NAME "Core levels")
set(CPACK_COMPONENT_MUSIC_DISPLAY_NAME "Music files")
set(CPACK_COMPONENT_SOUNDS_DISPLAY_NAME "Sound files")
set(CPACK_COMPONENT_WORLDS_DISPLAY_NAME "Core worlds")

# Descriptions
set(CPACK_COMPONENT_BASE_DESCRIPTION "The tsc program itself and its core requirements.")
set(CPACK_COMPONENT_CAMPAIGNS_DESCRIPTION "Standard campaigns included with TSC.")
set(CPACK_COMPONENT_LEVELS_DESCRIPTION "Standard levels included with TSC.")
set(CPACK_COMPONENT_MUSIC_DESCRIPTION "Background music files. No background music without this.")
set(CPACK_COMPONENT_SOUNDS_DESCRIPTION "Sound files. No sounds without this.")
set(CPACK_COMPONENT_WORLDS_DESCRIPTION "Standard worlds included with TSC.")

# Interdependencies
set(CPACK_COMPONENT_CAMPAIGNS_DEPENDS worlds)
set(CPACK_COMPONENT_WORLDS_DEPENDS levels)

# Required marks
set(CPACK_COMPONENT_BASE_REQUIRED ON)

# Misc
set(CPACK_RESOURCE_FILE_LICENSE "${TSC_SOURCE_DIR}/docs/license.txt")
set(CPACK_CREATE_DESKTOP_LINKS tsc)
set(CPACK_NSIS_MODIFY_PATH "ON") # Required for desktop shortcuts
set(CPACK_NSIS_MUI_ICON "${TSC_SOURCE_DIR}/extras/icons/windows_icon.ico") # Installer icon
set(CPACK_NSIS_MUI_UNIICON "${TSC_SOURCE_DIR}/extras/icons/windows_icon.ico") # Uninstaller icon
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\tsc.exe") # Icon for system control panel
set(CPACK_PACKAGE_ICON "${TSC_SOURCE_DIR}/extras/icons/windows_icon.ico") # Installer window bar icon

include(CPack)
