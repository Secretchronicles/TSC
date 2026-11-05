#############################################################################
# CMakeLists.txt  -  Build file
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

########################################
# Configuring CMake

# 3.5, not 3.0: CMake 4 REMOVED compatibility with anything below 3.5, so a
# cmake_minimum_required of 3.0 is a hard error there - "Compatibility with
# CMake < 3.5 has been removed from CMake" - and every Windows, macOS and Ubuntu
# 26.04 build stopped on this line before compiling anything.
#
# 3.5 and not higher on purpose. The number decides which policies default to
# NEW, so raising it further changes behaviour rather than only satisfying the
# parser, and this is exactly the value the project already forces on CEGUI
# through -DCMAKE_POLICY_VERSION_MINIMUM=3.5 in ProvideCEGUI.cmake.
cmake_minimum_required(VERSION 3.5)
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
set(TSC_VERSION_MINOR 2)
set(TSC_VERSION_PATCH 0)
set(TSC_VERSION_POSTFIX "beta2")
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
option(ENABLE_SCRIPT_DOCS "Build the scripting documentation" OFF)
option(USE_SYSTEM_PODPARSER "Use the system's pod-cpp library" OFF)
option(USE_SYSTEM_MRUBY "Use the system's mruby library" OFF)
option(USE_SYSTEM_CEGUI "Use the system's CEGUI library" OFF)
option(USE_LIBXMLPP3 "Use libxml++3.0 instead of libxml++2.6 (experimental)" OFF)

########################################
# Compiler config

# We use C++17 for SFML 3 compatibility. For portability it's important to not rely on
# compiler-specific extensions, so we'll disable them.
set(CMAKE_CXX_STANDARD 17)
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

# macOS's libc++ removed std::auto_ptr in C++17, but libxml++ 2.6's headers
# (saxparser.h, textreader.h, xsdvalidator.h, ...) declare `std::auto_ptr<Impl>
# pimpl_;`, so compiling any translation unit that includes them fails under
# -std=c++17 with "no template named 'auto_ptr' in namespace 'std'". Homebrew
# ships only libxml++ 2.6 (no 3.0/5.0 formula), and from libc++ 18 not even the
# opt-in _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR macro brings it back, so the robust
# fix is a tiny libc++-only std::auto_ptr shim, force-included before any header.
# Linux's libstdc++ keeps std::auto_ptr, so the shim is libc++-guarded and never
# redefines an existing one. See src/core/libxmlpp_autoptr_compat.hpp.
if (APPLE)
  add_compile_options(-include "${TSC_SOURCE_DIR}/src/core/libxmlpp_autoptr_compat.hpp")
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
endif()

find_package(SFML COMPONENTS audio graphics window system REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

if (USE_SYSTEM_CEGUI)
  find_package(CEGUI COMPONENTS OpenGL REQUIRED)
else()
  message(STATUS "Compiling CEGUI statically")
  include("ProvideCEGUI")
  # CEGUI is built as STATIC libraries here, so its headers must not declare their
  # symbols __declspec(dllimport). Without -DCEGUI_STATIC every CEGUI symbol
  # becomes an __imp_* import the static libs do not provide, and the Windows
  # (MinGW) link fails with "undefined reference to `__imp__ZN5CEGUI...'" for
  # String, System, OpenGLRendererBase, Exception and friends. CEGUI's own
  # FindCEGUI sets this from pkg-config, but the static ProvideCEGUI path did not,
  # so it was missing. It is a no-op where there is no dllimport (Linux/macOS), so
  # define it unconditionally in the static path.
  add_definitions(-DCEGUI_STATIC)
endif()

if (USE_LIBXMLPP3)
  find_package(LibXmlPP 3.0 REQUIRED)
else()
  find_package(LibXmlPP 2.6 REQUIRED)
endif()

find_package(Boost 1.50.0 COMPONENTS filesystem chrono thread REQUIRED)
set(Boost_COMPONENTS Boost::filesystem Boost::chrono Boost::thread)

# Libraries we can build ourselves under certain cirumstances if missing
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
# Compiler flags by dependencies

include_directories(
  ${SFML_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${LibXmlPP_INCLUDE_DIRS}
  ${PodParser_INCLUDE_DIRS}
  ${CEGUI_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${MRuby_INCLUDE_DIR}
  ${X11_INCLUDE_DIR}
  ${PNG_INCLUDE_DIRS})

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
  ${CEGUI_LIBRARIES}
  ${SFML_LIBRARIES}
  ${SFML_DEPENDENCIES}
  ${Boost_COMPONENTS}
  ${OPENGL_LIBRARIES}
  ${PNG_LIBRARIES}
  ${MRuby_LIBRARIES}
  ${LibXmlPP_LIBRARIES})

# Passing --as-needed to ld ensures that we get the Win32 ld's behaviour
# even on Linux and discover linking problems before building for Win32.
#
# It is a GNU ld and lld option, and Apple's linker does not have it. Passing it
# there is not ignored, it is FATAL - every macOS build died at the very last
# step, after compiling all 203 objects:
#
#   [203/203] Linking CXX executable tsc
#   ld: unknown options: --as-needed
#   clang++: error: linker command failed with exit code 1
#
# on macOS 15 x86_64, macOS 15 arm64 and macOS 26 arm64 alike, which is why a
# release that carried 97 assets carried no disk image at all.
#
# So ask the LINKER whether it knows the flag, by actually linking with it,
# rather than deciding from the platform: the flag belongs to the linker, and
# which linker is in use is not something CMAKE_SYSTEM_NAME answers - lld on a
# Mac would take it, and a cross-build might not. check_linker_flag would say
# this in one line but it needs CMake 3.18, and this project supports 3.5.
include(CheckCXXSourceCompiles)

# Apple's equivalent. -dead_strip_dylibs drops the load commands for dylibs
# nothing actually references, which is the part of --as-needed that finds the
# over-linking this flag is here to find.
foreach(_tsc_link_flag "-Wl,--as-needed" "-Wl,-dead_strip_dylibs")
  if (NOT DEFINED TSC_LINK_TRIM_FLAG)
    string(MAKE_C_IDENTIFIER "TSC_LINKER_ACCEPTS${_tsc_link_flag}" _tsc_link_var)
    set(_tsc_saved_flags "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${_tsc_link_flag}")
    check_cxx_source_compiles("int main(void) { return 0; }" ${_tsc_link_var})
    set(CMAKE_REQUIRED_FLAGS "${_tsc_saved_flags}")
    if (${_tsc_link_var})
      set(TSC_LINK_TRIM_FLAG "${_tsc_link_flag}")
    endif()
  endif()
endforeach()

if (DEFINED TSC_LINK_TRIM_FLAG)
  message(STATUS "Trimming unused libraries with ${TSC_LINK_TRIM_FLAG}")
  set_property(TARGET tsc APPEND PROPERTY LINK_FLAGS "${TSC_LINK_TRIM_FLAG}")
else()
  # Not a reason to fail: the flag is a check, not something the game needs to
  # run. Say so, so a linker nobody tried yet is a line in the log and not a
  # silently weaker build.
  message(STATUS "This linker takes neither --as-needed nor -dead_strip_dylibs;"
                 " unused libraries will not be trimmed.")
endif()

if (WIN32)
  target_link_libraries(tsc iconv intl ws2_32)
else()
  target_link_libraries(tsc
    ${X11_LIBRARIES}
    ${CMAKE_DL_LIBS})
  if (CMAKE_SYSTEM_NAME MATCHES "BSD" OR APPLE)
    # These were named as bare -liconv and -lintl, which asks the linker to
    # find them on its own default search path. On the BSDs they are not on it:
    # the ports tree installs them under /usr/local/lib, and the link failed
    # with "ld: error: unable to find library -liconv" on a machine that had
    # them installed all along. Every other dependency in this file is resolved
    # to a full path by a find_package before being linked; these two are now
    # resolved the same way. FindLibIntl.cmake is this project's own module;
    # FindIconv is CMake's own, shipped since CMake 3.11, and on systems where
    # iconv lives in libc it correctly answers with no library to link at all.
    # Every current BSD, and every macOS runner, is far past 3.11.
    #
    # macOS is here for the same reason as the BSDs and not by analogy. On
    # Linux the gettext functions are IN glibc, so nothing needs linking and
    # nobody notices; on macOS they are not in libSystem, they come from
    # Homebrew's gettext, and the link ends with:
    #
    #   "_libintl_ngettext", referenced from:
    #       TSC::cEditor::replace_sprites() in editor.cpp.o
    #   "_libintl_setlocale", referenced from: TSC::I18N_Init() in i18n.cpp.o
    #   "_libintl_textdomain", referenced from: TSC::I18N_Init() in i18n.cpp.o
    #   ld: symbol(s) not found for architecture arm64
    #
    # on all three macOS jobs. That was hidden until now: the build never got
    # as far as resolving symbols, because -Wl,--as-needed had already been
    # refused by Apple's linker at the start of the same step.
    #
    # Homebrew's gettext is KEG-ONLY - macOS ships a BSD gettext of its own and
    # Homebrew refuses to shadow it - so `brew install gettext` does NOT put
    # libintl into $(brew --prefix)/lib, which is the only place
    # CMAKE_PREFIX_PATH pointed. The workflow passes the keg's own prefix as
    # well now, and the hint below means a plain local `cmake` finds it too.
    if (APPLE)
      find_program(TSC_BREW brew)
      if (TSC_BREW)
        execute_process(COMMAND "${TSC_BREW}" --prefix gettext
                        OUTPUT_VARIABLE TSC_GETTEXT_PREFIX
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET)
        if (TSC_GETTEXT_PREFIX)
          message(STATUS "Homebrew gettext (keg-only) at ${TSC_GETTEXT_PREFIX}")
          list(APPEND CMAKE_PREFIX_PATH "${TSC_GETTEXT_PREFIX}")
        endif()
      endif()
    endif()
    find_package(Iconv REQUIRED)
    find_package(LibIntl REQUIRED)
    target_link_libraries(tsc ${Iconv_LIBRARIES} ${LIBINTL_LIBRARIES})
    # main() finds the game data inside the .app with CFBundleGetMainBundle()
    # and friends, which are in CoreFoundation. Nothing linked it by name: it
    # used to arrive through another library's dependencies, and with
    # -dead_strip_dylibs and the current dependency set it no longer does, so
    # both macOS jobs ended with
    #   "_CFBundleCopyBundleURL", referenced from: _main in main.cpp.o
    #   ld: symbol(s) not found for architecture arm64
    if (APPLE)
      find_library(TSC_COREFOUNDATION CoreFoundation REQUIRED)
      target_link_libraries(tsc ${TSC_COREFOUNDATION})
    endif()
  endif()
endif()

if (NOT USE_SYSTEM_MRUBY)
  add_dependencies(tsc mruby)
endif()

if (NOT USE_SYSTEM_CEGUI)
  add_dependencies(tsc cegui)
endif()

if (ENABLE_SCRIPT_DOCS)
  add_executable(scrdg ${scrdg_sources})
  target_link_libraries(scrdg ${Boost_COMPONENTS} ${PodParser_LIBRARIES})

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
  # CEGUI's CoreWindowRendererSet is loaded at RUNTIME, so GetPrerequisites does
  # not see it in tsc.exe and it used to be appended to the list by name. That
  # name came from CEGUI_CoreWindowRendererSet_LIBRARY, which only FindCEGUI.cmake
  # sets - i.e. only when CEGUI is FOUND on the system. Windows builds CEGUI here
  # instead (ProvideCEGUI.cmake, USE_SYSTEM_CEGUI defaults to OFF), so the
  # variable was empty and, expanded at configure time into the install script,
  # generated
  #
  #     get_filename_component(CEGUI_CoreWindowRendererSet  NAME)
  #
  # - two arguments where three are required. Every win64 package died there:
  # "CMake Error at .../cmake_install.cmake:57 (get_filename_component): called
  # with incorrect number of arguments", then "CPack Error: Error when generating
  # package: TSC", after a build that had otherwise succeeded.
  #
  # So the name is resolved HERE, where it can be tested, and the append is
  # skipped when there is nothing to append. That is also the truth of the
  # Windows build: CEGUI is linked STATICALLY into tsc.exe (CEGUI_STATIC), so the
  # window-renderer set is already inside the executable and there is no separate
  # module to install beside it.
  set(TSC_EXTRA_RUNTIME_LIB "")
  if(CEGUI_CoreWindowRendererSet_LIBRARY)
    get_filename_component(TSC_EXTRA_RUNTIME_LIB
      "${CEGUI_CoreWindowRendererSet_LIBRARY}" NAME)
    string(REPLACE ".a" "" TSC_EXTRA_RUNTIME_LIB "${TSC_EXTRA_RUNTIME_LIB}")
  endif()
  install(CODE
    "include(GetPrerequisites)
    get_prerequisites(\"${CMAKE_CURRENT_BINARY_DIR}/tsc.exe\" LIBLIST 1 1 \"\" \"\" )
    if(NOT \"${TSC_EXTRA_RUNTIME_LIB}\" STREQUAL \"\")
      list(APPEND LIBLIST \"${TSC_EXTRA_RUNTIME_LIB}\") # not detected, but still required
    endif()
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

if (ENABLE_SCRIPT_DOCS)
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
    RENAME org.secretchronicles.TSC.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_32.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps
    RENAME org.secretchronicles.TSC.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_48.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
    RENAME org.secretchronicles.TSC.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_64.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
    RENAME org.secretchronicles.TSC.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/icons/tsc_icon_128.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
    RENAME org.secretchronicles.TSC.png
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/org.secretchronicles.TSC.desktop"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    COMPONENT base)
  install(FILES "${TSC_SOURCE_DIR}/extras/org.secretchronicles.TSC.metainfo.xml"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
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
message(STATUS "Use system-provided pod-cpp:       ${USE_SYSTEM_PODPARSER}")
message(STATUS "Use system-provided mruby:         ${USE_SYSTEM_MRUBY}")
message(STATUS "Use system-provided CEGUI:         ${USE_SYSTEM_CEGUI}")

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
