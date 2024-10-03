/***************************************************************************
 * global_basic.h
 *
 * Copyright © 2003 - 2011 Florian Richter
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_GLOBAL_BASIC_HPP
#define TSC_GLOBAL_BASIC_HPP

#ifdef _WIN32
// We use some features from IE 5, sadly.
#define _WIN32_IE 0x0500
#endif

/* uncomment to enable experimental rendering in a thread
* still randomly crashes because gl functions are called while the render thread is active
*/
//#define TSC_RENDER_THREAD_TEST

/* *** *** *** *** *** *** *** Debugging *** *** *** *** *** *** *** *** *** *** */

#if defined(_MSC_VER) && defined(_DEBUG)
// disable possible loss of data
#pragma warning ( disable : 4267 )
#endif

// debug printf macro
// prints to error stream using fprintf
#ifdef _DEBUG
#define debug_print(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#else
#define debug_print(format, ...)
#endif

/* *** *** *** *** *** *** *** Includes *** *** *** *** *** *** *** *** *** *** */

// C stdlib
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cctype>
#include <sys/stat.h>
#include <sys/types.h>

// C++ STL
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <time.h>
#include <math.h>
#include <functional>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <utility>
#include <iomanip>
#include <stack>

// TSC build configuration header
#include "config.hpp"

// Boost
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/system/error_code.hpp>

// glibmm (we use a single helper function from it, filename_from_utf8(),
// to support Unicode pathes -- maybe we can go without it?)
#include <glibmm/convert.h>

// libxml++
#include <libxml++/libxml++.h>

// CEGUI
// Must be included before X11, which have #defines such as
// None, True, and False that screw CEGUI declarations.
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GLRenderer.h>
#include <CEGUI/ImageCodecModules/DevIL/ImageCodec.h>

#ifdef CEGUI_USE_EXPAT
#include <CEGUI/XMLParserModules/Expat/XMLParserModule.h>
#else
#include <CEGUI/XMLParserModules/Libxml2/XMLParserModule.h>
#endif

// SFML
// Must also be included before X11, which has a #define Status int that messes
// with SFML's Status enum.
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Audio.hpp>

// Other libs
#include <png.h>
#include "gettext.h"

// System-specific includes
#ifdef __unix__
#include <GL/glx.h>
#endif

#ifdef _WIN32
#include <shlobj.h>
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#endif

// MRuby
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/proc.h>
#include <mruby/range.h>

#ifndef PNG_COLOR_TYPE_RGBA
#define PNG_COLOR_TYPE_RGBA PNG_COLOR_TYPE_RGB_ALPHA
#endif

using std::vector;
using std::ifstream;
using std::fstream;
using std::ofstream;
using std::stringstream;
using std::ios;

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#endif
