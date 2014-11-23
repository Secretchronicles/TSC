/***************************************************************************
 * mrb_tsc.cpp - TSC mruby module
 *
 * Copyright © 2013-2014 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mrb_tsc.hpp"
#include "../../core/game_core.hpp"
#include "../../core/property_helper.hpp"
#include "../../core/filesystem/resource_manager.hpp"
#include "../../core/framerate.hpp"

/**
 * Module: TSC
 *
 * Module encapsulating stuff related to the game itself.
 */

using namespace TSC;
namespace fs = boost::filesystem;


/**
 * Method: TSC::require
 *
 *   require( path )
 *
 * Minimalistic file loading capability. Loads a file
 * relative to TSC’s scripting/ directory into the running
 * MRuby instance.
 *
 * Using this outside of the initialisation sequence doesn’t
 * make much sense.
 */
static mrb_value Require(mrb_state* p_state, mrb_value self)
{
    using namespace TSC;

    // Get the path argument
    char* cpath = NULL;
    mrb_get_args(p_state, "z", &cpath);

    // Append ".rb" and convert to a platform-independent boost path
    std::string spath(cpath);
    spath.append(".rb");
    fs::path path = utf8_to_path(spath);

    // Disallow absolute pathes, we don’t want load external files
    // accidentally
    if (path.is_absolute())
        mrb_raise(p_state, MRB_ARGUMENT_ERROR(p_state), "Absolute paths are not allowed.");

    // Open the MRuby file for reading
    fs::path scriptfile = pResource_Manager->Get_Game_Scripting_Directory() / path;
    fs::ifstream file(scriptfile);
    debug_print("require: Loading '%s'\n", path_to_utf8(scriptfile).c_str());
    if (!file.is_open())
        mrb_raisef(p_state, MRB_RUNTIME_ERROR(p_state), "Cannot open file '%s' for reading", scriptfile.generic_string().c_str());

    // Read it
    std::string code = readfile(file);
    file.close();

    // Create our context for exception handling
    mrbc_context* p_context = mrbc_context_new(p_state);
    p_context->capture_errors = true;
    p_context->lineno = 1;
    mrbc_filename(p_state, p_context, path_to_utf8(scriptfile.filename()).c_str());

    // Compile and run the MRuby code
    mrb_load_nstring_cxt(p_state, code.c_str(), code.length(), p_context);

    // Check for exceptions
    if (p_state->exc)
        mrb_exc_raise(p_state, mrb_obj_value(p_state->exc)); // Reraise

    // Finish
    mrbc_context_free(p_state, p_context);
    return mrb_nil_value();
}

/**
 * Method: TSC::platform
 *
 *   platform() → a_string
 *
 * Information about the platform TSC was compiled for.
 *
 * #### Return value
 * One of `:win32`, `:linux`, `:apple`, or `:other`. Please
 * notify me if you get `:other` from this method.
 */
static mrb_value Platform(mrb_state* p_state, mrb_value self)
{
#ifdef _WIN32
    return TSC::Scripting::str2sym(p_state, "win32");
#elif __linux
    return TSC::Scripting::str2sym(p_state, "linux");
#elif __APPLE__
    return TSC::Scripting::str2sym(p_state, "apple");
#else
    return TSC::Scripting::str2sym(p_state, "other");
#endif
}

/**
 * Method: TSC::quit
 *
 *   quit()
 *
 * Instructs TSC to terminate the main loop, exiting the
 * game as if regularily closed.
 */
static mrb_value Quit(mrb_state* p_state, mrb_value self)
{
    game_exit = true;
    return mrb_nil_value();
}

/**
 * Method: TSC::exit
 *
 *   exit( exitcode )
 *
 * Immediately and forcibly terminates TSC without any
 * cleanup.
 *
 * `0 <= exitcode < 255` is the range for `exitcode`.
 */
static mrb_value Exit(mrb_state* p_state, mrb_value self)
{
    mrb_int exitcode;
    mrb_get_args(p_state, "i", &exitcode);

    exit(exitcode);
    return mrb_nil_value(); // Not reached
}

/**
 * Method: TSC::current_framerate
 *
 *   current_framerate() → integer
 *
 * The current frames per second (FPS).
 */
static mrb_value Current_Framerate(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pFramerate->m_fps);
}

/**
 * Method: TSC::average_framerate
 *
 *   average_framerate() → integer
 *
 * The average frames per second (FPS).
 */
static mrb_value Average_Framerate(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pFramerate->m_fps_average);
}

/**
 * Method: TSC::best_framerate
 *
 *   best_framerate() → integer
 *
 * The best frames per second (FPS).
 */
static mrb_value Best_Framerate(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pFramerate->m_fps_best);
}

/**
 * Method: TSC::worst_framerate
 *
 *   worst_framerate() → integer
 *
 * The worst frames per second (FPS).
 */
static mrb_value Worst_Framerate(mrb_state* p_state, mrb_value self)
{
    return mrb_fixnum_value(pFramerate->m_fps_worst);
}

/**
 * Method: TSC::version
 *
 *   version() → a_string
 *
 * TSC’s version number in the form `major.minor.patch`.
 */
static mrb_value Version(mrb_state* p_state, mrb_value self)
{
    std::stringstream ss;

    ss << TSC_VERSION_MAJOR << "." << TSC_VERSION_MINOR << "." << TSC_VERSION_PATCH;

    return mrb_str_new_cstr(p_state, ss.str().c_str());
}

/**
 * Method: TSC::debug_mode?
 *
 *   debug_mode?() → true or false
 *
 * Checks if this TSC has been compiled in debug mode, and if so,
 * returns `true`, `false` otherwise.
 */
static mrb_value Is_Debug_Mode(mrb_state* p_state, mrb_value self)
{
#ifdef _DEBUG
    return mrb_true_value();
#else
    return mrb_false_value();
#endif
}

void TSC::Scripting::Init_TSC(mrb_state* p_state)
{
    struct RClass* p_rmTSC = mrb_define_module(p_state, "TSC");

    mrb_define_module_function(p_state, p_rmTSC, "require", Require, MRB_ARGS_REQ(1));
    mrb_define_module_function(p_state, p_rmTSC, "platform", Platform, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "quit", Quit, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "exit", Exit, MRB_ARGS_REQ(1));
    mrb_define_module_function(p_state, p_rmTSC, "current_framerate", Current_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "average_framerate", Average_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "best_framerate", Best_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "worst_framerate", Worst_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "version", Version, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "debug_mode?", Is_Debug_Mode, MRB_ARGS_NONE());
}
