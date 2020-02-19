/***************************************************************************
 * mrb_tsc.cpp - TSC mruby module
 *
 * Copyright © 2012-2017 The TSC Contributors
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
#include "../../gui/game_console.hpp"

/**
 * Module: TSC
 *
 * Module encapsulating stuff related to the game itself.
 */

using namespace TSC;
namespace fs = boost::filesystem;

/**
 * Method: TSC::platform
 *
 *   platform() → a_string
 *
 * Information about the platform TSC was compiled for.
 *
 * =head4 Return value
 *
 * One of :win32, :linux, :apple, or :other. Please
 * notify us if you get :other from this method.
 */
static mrb_value Platform(mrb_state* p_state, mrb_value self)
{
#ifdef _WIN32
    return TSC::Scripting::str2sym(p_state, "win32");
#elif __linux__
    return TSC::Scripting::str2sym(p_state, "linux");
#elif __APPLE__
    return TSC::Scripting::str2sym(p_state, "apple");
#elif __FreeBSD__
    return TSC::Scripting::str2sym(p_state, "freebsd");
#elif __OpenBSD__
    return TSC::Scripting::str2sym(p_state, "openbsd");
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
 * C<< 0 E<lt>= exitcode E<lt> 255 >> is the range for C<exitcode>.
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
 * TSC’s version number in the form C<major.minor.patch>.
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
 * returns C<true>, C<false> otherwise.
 */
static mrb_value Is_Debug_Mode(mrb_state* p_state, mrb_value self)
{
#ifdef _DEBUG
    return mrb_true_value();
#else
    return mrb_false_value();
#endif
}

/*
 * Internal method used to implement #puts et al.
 */
static mrb_value printstr__(mrb_state* p_state, mrb_value self)
{
    mrb_value arg;

    mrb_get_args(p_state, "o", &arg);

    // This method is only called internally and is guaranteed to
    // get passed a string.
    gp_game_console->Append_Text(std::string(RSTRING_PTR(arg), RSTRING_LEN(arg)));

    return arg;
}

void TSC::Scripting::Init_TSC(mrb_state* p_state)
{
    struct RClass* p_rmTSC = mrb_define_module(p_state, "TSC");

    mrb_define_module_function(p_state, p_rmTSC, "platform", Platform, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "quit", Quit, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "exit", Exit, MRB_ARGS_REQ(1));
    mrb_define_module_function(p_state, p_rmTSC, "current_framerate", Current_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "average_framerate", Average_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "best_framerate", Best_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "worst_framerate", Worst_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "version", Version, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmTSC, "debug_mode?", Is_Debug_Mode, MRB_ARGS_NONE());

    /* Cleanly remove the Kernel#__printstr__ method provided by the mruby-print MRBGEM
     * and instead overwrite it with our own. The mruby-print MRBGEM implements #puts et
     * al. all on top of this method, so that one can take advantage of this abstraction
     * by simply rewriting that method to use TSC's game console. */
    mrb_undef_method(p_state, p_state->kernel_module, "__printstr__");
    mrb_define_method(p_state, p_state->kernel_module, "__printstr__", printstr__, MRB_ARGS_REQ(1));
}
