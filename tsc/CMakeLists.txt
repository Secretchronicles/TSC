#############################################################################
# CMakeLists.txt  -  Build file
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

########################################
# Configuring CMake

cmake_minimum_required(VERSION 3.13)
project(TSC C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
include(ExternalProject)
include(GNUInstallDirs)

########################################
# TSC version number

# If you get a merge conflict below, always keep "dev" on the
# "devel" branch. The postfix is only to be altered on release
# branches.
set(TSC_VERSION_MAJOR 2)
set(TSC_VERSION_MINOR 1)
set(TSC_VERSION_PATCH 1)
set(TSC_VERSION_POSTFIX "dev")
string(TIMESTAMP TSC_COMPILE_YEAR "%Y")
string(TIMESTAMP TSC_COMPILE_MONTH "%m")
string(TIMESTAMP TSC_COMPILE_DAY "%d")

set (TSC_VERSION "${TSC_VERSION_MAJOR}.${TSC_VERSION_MINOR}.${TSC_VERSION_PATCH}${TSC_VERSION_POSTFIX}")

# Set TSC_VERSION_GIT if possible
execute_process(COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${TSC_SOURCE_DIR}
  OUTPUT_VARIABLE TSC_VERSION_GIT
  OUTPUT_STRIP_TRAILING_WHITESPACE)

###############################################
# Flags & Options

option(ENABLE_NLS "Enable translations and localisations" ON)
option(ENABLE_SCRIPT_DOCS "Build the scripting documentation" ON)
option(USE_SYSTEM_TINYCLIPBOARD "Use the system's tinyclipboard library" OFF)
option(USE_SYSTEM_PODPARSER "Use the system's pod-cpp library" OFF)
option(USE_LIBXMLPP3 "Use libxml++3.0 instead of libxml++2.6 (experimental)" OFF)

########################################
# Compiler config

# We use some C++11. For portability it's important to not rely on
# compiler-specific extensions, so we'll disable them.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include CEGUI's weird location into the runtime library search path.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# -Wall for better code quality, and -Wno-deprecated-delcarations to prevent
# useless warnings from not-yet-c++11-ready dependencies (glibmm!).
# The _DEBUG macro is used in TSC's own codebase for debugging code.
add_compile_options(-Wall -Wno-deprecated-declarations)
if (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_compile_options(-fno-inline)
  add_definitions(-D_DEBUG=1)
endif()

if(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "$ENV{systemdrive}/Games/${CMAKE_PROJECT_NAME}" CACHE PATH "CMAKE PREFIX" FORCE)
endif()

########################################
# Dependencies

# Programs used in the build process
find_package(PkgConfig REQUIRED)

if (ENABLE_NLS)
  find_package(Gettext)
endif()

# External libraries we depend on directly and programs we use in the
# build process.

# Indicate we need legacy OpenGL, not GLVND
set(OpenGL_GL_PREFERENCE LEGACY)

# Boost needs special treatment.
if (WIN32)
  # Boost thread library has a different name on Windows to indicate
  # the different implemention. Just complicates the build system...
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_THREADAPI win32)
else()
  # SFML requires us to link in -lX11 ourselves for whatever reason (otherwise
  # linking error).
  find_package(X11 REQUIRED)

  if(NOT TARGET X11::X11)
    add_library(X11::X11 INTERFACE)
    set_target_properties(X11::X11 PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${X11_INCLUDE_DIR}"
                          INTERFACE_LINK_LIBRARIES "${X11_LIBRARIES}")
  endif()
endif()

find_package(SFML COMPONENTS audio graphics window system REQUIRED)
set(SFML_COMPONENTS SFML::audio SFML::graphics SFML::window SFML::system)

find_package(CEGUI COMPONENTS OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(PCRE REQUIRED)

if (USE_LIBXMLPP3)
  find_package(LibXmlPP 3.0 REQUIRED)
else()
  find_package(LibXmlPP 2.6 REQUIRED)
endif()

find_package(OpenGL REQUIRED)

find_package(Boost 1.50.0 COMPONENTS filesystem chrono thread REQUIRED)
set(Boost_COMPONENTS Boost::filesystem Boost::chrono Boost::thread)

# Libraries we can build ourselves under certain cirumstances if missing
include("ProvideTinyclipboard")
include("ProvidePodParser")
include("ProvideMRuby")

########################################
# Configuration header

configure_file(${TSC_SOURCE_DIR}/src/core/config.hpp.in
  ${TSC_BINARY_DIR}/config.hpp)
include_directories(${TSC_BINARY_DIR})

########################################
# Credits file

file(READ ${TSC_SOURCE_DIR}/docs/authors.txt TSC_CREDITS_STR OFFSET 457)
string(STRIP "${TSC_CREDITS_STR}" TSC_CREDITS_STR)
string(REPLACE "\n" "\\n" TSC_CREDITS_STR "${TSC_CREDITS_STR}")
configure_file(${TSC_SOURCE_DIR}/src/core/credits.cpp.in ${TSC_BINARY_DIR}/credits.cpp)

########################################
# Source files

file(GLOB_RECURSE tsc_sources
  "src/*.cpp"
  "src/*.hpp")

# Windows icon resource
# See http://stackoverflow.com/a/708382
if (WIN32)
  enable_language(RC)

  configure_file("${TSC_SOURCE_DIR}/src/gui/icon.rc.in" "${TSC_BINARY_DIR}/icon.rc")
  list(APPEND tsc_sources "${TSC_BINARY_DIR}/icon.rc")
endif()

file(GLOB_RECURSE scrdg_sources
  "scrdg/*.cpp"
  "scrdg/*.hpp")

file(GLOB_RECURSE scriptdoc_sources
  "src/scripting/*.cpp"
  "src/scripting/*.hpp"
  "data/scripting/*.rb"
  "docs/scripting/*")

########################################
# Gettext internationalization

if (ENABLE_NLS)
  # Process all PO files into MO files usable at runtime.
  file(GLOB po_files "data/translations/*.po")
  gettext_process_po_files(languages ALL PO_FILES ${po_files})
endif()

########################################
# Main targets

add_executable(tsc ${tsc_sources} ${TSC_BINARY_DIR}/credits.cpp)

target_link_libraries(tsc
  CEGUI::CEGUI
  ${SFML_COMPONENTS}
  ${Boost_COMPONENTS}
  OpenGL::GLU
  OpenGL::OpenGL
  PNG::PNG
  MRuby::MRuby
  Tinyclipboard::Tinyclipboard
  LibXmlPP::LibXmlPP
  PCRE::PCRE)

# Passing --as-needed to ld ensures that we get the Win32 ld's behaviour
# even on Linux and discover linking problems before building for Win32.
set_property(TARGET tsc APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")

if (WIN32)
  target_link_libraries(tsc iconv intl ws2_32)
else()
  target_link_libraries(tsc
    OpenGL::GLX
    X11::X11
    ${CMAKE_DL_LIBS})
  if (CMAKE_SYSTEM_NAME MATCHES "BSD")
    target_link_libraries(tsc iconv intl)
  endif()
endif()

add_dependencies(tsc mruby)

if (ENABLE_SCRIPT_DOCS)
  add_executable(scrdg ${scrdg_sources})
  target_link_libraries(scrdg ${Boost_COMPONENTS} PodParser::PodParser)

  add_custom_command(OUTPUT "${TSC_BINARY_DIR}/scriptdocs"
    COMMAND scrdg "${TSC_BINARY_DIR}/scriptdocs" "${TSC_SOURCE_DIR}" ${TSC_VERSION} ${TSC_VERSION_GIT}
    DEPENDS ${scriptdoc_sources}
    VERBATIM)
  add_custom_target(scriptdocumentation
    DEPENDS "${TSC_BINARY_DIR}/scriptdocs")

  add_dependencies(tsc scriptdocumentation)
endif()

########################################
# Installation instructions

install(TARGETS tsc
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT base)

if(WIN32)
  install(CODE
    "include(GetPrerequisites)
    get_prerequisites(\"${CMAKE_CURRENT_BINARY_DIR}/tsc.exe\" LIBLIST 1 1 \"\" \"\" )
    get_filename_component(CEGUI_CoreWindowRendererSet ${CEGUI_CoreWindowRendererSet_LIBRARY} NAME)
    string(REPLACE \".a\" \"\" CEGUI_CoreWindowRendererSet \${CEGUI_CoreWindowRendererSet})
    list(APPEND LIBLIST \${CEGUI_CoreWindowRendererSet}) # Not detected, but still required
    foreach(LIB \${LIBLIST})
      gp_resolve_item(\"${CMAKE_CURRENT_BINARY_DIR}/tsc.exe\" \"\${LIB}\" \"\" \"\" resolved_item)
      list(APPEND LIBS \${resolved_item})
    endforeach()
	file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\" TYPE FILE FILES \${LIBS})"
    COMPONENT base)
endif()

install(DIRECTORY "${TSC_SOURCE_DIR}/data/campaigns/" # Note trailing slash for content copy
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/campaigns
  COMPONENT campaigns)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/editor/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/editor
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/gui/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/gui
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/icon/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/icon
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/levels/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/levels
  COMPONENT levels)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/music/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/music
  COMPONENT music)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/pixmaps/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/pixmaps
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/schema/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/schema
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/scripting/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/scripting
  COMPONENT base)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/sounds/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/sounds
  COMPONENT sounds)
install(DIRECTORY "${TSC_SOURCE_DIR}/data/worlds/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/worlds
  COMPONENT worlds)

if(ENABLE_SCRIPT_DOCS)
  install(DIRECTORY "${TSC_BINARY_DIR}/scriptdocs"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/tsc
    COMPONENT base)
endif()

if(NOT WIN32)
  install(FILES "${TSC_SOURCE_DIR}/docs/man/tsc.6"
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man6
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_16.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps
    RENAME tsc.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_32.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps
    RENAME tsc.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_48.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
    RENAME tsc.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_64.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
    RENAME tsc.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_128.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    RENAME tsc.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/tsc.desktop"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/tsc.appdata.xml"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/appdata
    COMPONENT base)
endif()

if (ENABLE_NLS)
  foreach(pofile ${po_files})
    get_filename_component(lang ${pofile} NAME_WE)

    install(FILES "${TSC_BINARY_DIR}/${lang}.gmo" # .gmo is generated by gettext_process_po_files() further above
      DESTINATION ${CMAKE_INSTALL_DATADIR}/tsc/translations/${lang}/LC_MESSAGES
      RENAME "Secretchronicles.mo" # This must match TSC_GETTEXT_DOMAIN from global_game.hpp so Gettext finds the files
      COMPONENT base)
  endforeach()
endif()

########################################
# uninstall target

if(NOT TARGET uninstall)
  configure_file("${TSC_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${TSC_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${TSC_BINARY_DIR}/cmake_uninstall.cmake)
endif()

########################################
# CPack

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
set(CPACK_RESOURCE_FILE_LICENSE "${TSC_SOURCE_DIR}/../COPYING")
set(CPACK_CREATE_DESKTOP_LINKS tsc)
set(CPACK_NSIS_MODIFY_PATH "ON") # Required for desktop shortcuts
set(CPACK_NSIS_MUI_ICON "${TSC_SOURCE_DIR}/extras/icons/windows_icon.ico") # Installer icon
set(CPACK_NSIS_MUI_UNIICON "${TSC_SOURCE_DIR}/extras/icons/windows_icon.ico") # Uninstaller icon
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\tsc.exe") # Icon for system control panel
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_ICON "${TSC_SOURCE_DIR}/extras\\\\Header.bmp") # Should be a 24-bit BMP sized 150x70, Gimp doesn't work.

if(WIN32)
  set(CPACK_GENERATOR "7Z;NSIS")
endif()

include(CPack)

########################################
# Print summary

message(STATUS "--------------- Build info summary ----------------")
message(STATUS "TSC version:    ${TSC_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")

message(STATUS "--------------- Configuration summary -------------")
message(STATUS "Enable native language support:    ${ENABLE_NLS}")
message(STATUS "Enable the scripting API docs:     ${ENABLE_SCRIPT_DOCS}")
message(STATUS "Use system-provided tinyclipboard: ${USE_SYSTEM_TINYCLIPBOARD}")
message(STATUS "Use system-provided pod-cpp:       ${USE_SYSTEM_PODPARSER}")

message(STATUS "--------------- Path configuration -----------------")
message(STATUS "Install prefix:        ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Binary directory:      ${CMAKE_INSTALL_BINDIR}")
message(STATUS "Shared data directory: ${CMAKE_INSTALL_DATADIR}")
if(NOT WIN32)
  message(STATUS "Manpage directory:     ${CMAKE_INSTALL_MANDIR}")
endif()
message(STATUS "(relative pathes are relative to the install prefix;")
message(STATUS "the directory /tsc will be appended to the shared")
message(STATUS "data directory indicated above.)")
message(STATUS "----------------------------------------------------")
