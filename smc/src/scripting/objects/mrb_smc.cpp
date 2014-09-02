#include "mrb_smc.hpp"
#include "../../core/game_core.hpp"
#include "../../core/property_helper.hpp"
#include "../../core/filesystem/resource_manager.hpp"
#include "../../core/filesystem/package_manager.hpp"
#include "../../core/filesystem/vfs.hpp"
#include "../../core/framerate.hpp"

/**
 * Module: SMC
 *
 * Module encapsulating stuff related to the game itself.
 */

using namespace SMC;
namespace fs = boost::filesystem;


/**
 * Method: SMC::require
 *
 *   require( path [, package ] )
 *
 * Minimalistic file loading capability. Loads a file
 * relative to SMC’s scripting/ directory into the running
 * MRuby instance.  If a package is provided, it will search
 * the user and game package scripting directory for the script.
 *
 * Using this outside of the initialisation sequence doesn’t
 * make much sense.
 */
static mrb_value Require(mrb_state* p_state, mrb_value self)
{
    using namespace SMC;

    // Get the path argument
    char* cpath = NULL;
    char* cpackage = NULL;
    mrb_get_args(p_state, "z|z", &cpath, &cpackage);

    // Append ".rb" and convert to a platform-independent boost path
    std::string spath(cpath);
    spath.append(".rb");
    fs::path path = utf8_to_path(spath);

    // Disallow absolute pathes, we don’t want load external files
    // accidentally
    if (path.is_absolute())
        mrb_raise(p_state, MRB_ARGUMENT_ERROR(p_state), "Absolute paths are not allowed.");

    // Open the MRuby file for reading
    fs::path scriptfile = pPackage_Manager->Get_Scripting_Path(cpackage ? cpackage : "", spath);
    std::istream* s = pVfs->Open_Stream(scriptfile);
    debug_print("require: Loading '%s'\n", path_to_utf8(scriptfile).c_str());
    if (!s)
        mrb_raisef(p_state, MRB_RUNTIME_ERROR(p_state), "Cannot open file '%s' for reading", scriptfile.generic_string().c_str());

    // Read it
    std::string code = readfile(*s);
    delete s;

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
 * Method: SMC::platform
 *
 *   platform() → a_string
 *
 * Information about the platform SMC was compiled for.
 *
 * #### Return value
 * One of `:win32`, `:linux`, `:apple`, or `:other`. Please
 * notify me if you get `:other` from this method.
 */
static mrb_value Platform(mrb_state* p_state, mrb_value self)
{
#ifdef _WIN32
    return SMC::Scripting::str2sym(p_state, "win32");
#elif __linux
    return SMC::Scripting::str2sym(p_state, "linux");
#elif __APPLE__
    return SMC::Scripting::str2sym(p_state, "apple");
#else
    return SMC::Scripting::str2sym(p_state, "other");
#endif
}

/**
 * Method: SMC::quit
 *
 *   quit()
 *
 * Instructs SMC to terminate the main loop, exiting the
 * game as if regularily closed.
 */
static mrb_value Quit(mrb_state* p_state, mrb_value self)
{
    game_exit = true;
    return mrb_nil_value();
}

/**
 * Method: SMC::exit
 *
 *   exit( exitcode )
 *
 * Immediately and forcibly terminates SMC without any
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
 * Method: SMC::current_framerate
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
 * Method: SMC::average_framerate
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
 * Method: SMC::best_framerate
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
 * Method: SMC::worst_framerate
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
 * Method: SMC::version
 *
 *   version() → a_string
 *
 * SMC’s version number in the form `major.minor.patch`.
 */
static mrb_value Version(mrb_state* p_state, mrb_value self)
{
    std::stringstream ss;

    ss << SMC_VERSION_MAJOR << "." << SMC_VERSION_MINOR << "." << SMC_VERSION_PATCH;

    return mrb_str_new_cstr(p_state, ss.str().c_str());
}

/**
 * Method: SMC::debug_mode?
 *
 *   debug_mode?() → true or false
 *
 * Checks if this SMC has been compiled in debug mode, and if so,
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

void SMC::Scripting::Init_SMC(mrb_state* p_state)
{
    struct RClass* p_rmSMC = mrb_define_module(p_state, "SMC");

    mrb_define_module_function(p_state, p_rmSMC, "require", Require, MRB_ARGS_ARG(1,1));
    mrb_define_module_function(p_state, p_rmSMC, "platform", Platform, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "quit", Quit, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "exit", Exit, MRB_ARGS_REQ(1));
    mrb_define_module_function(p_state, p_rmSMC, "current_framerate", Current_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "average_framerate", Average_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "best_framerate", Best_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "worst_framerate", Worst_Framerate, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "version", Version, MRB_ARGS_NONE());
    mrb_define_module_function(p_state, p_rmSMC, "debug_mode?", Is_Debug_Mode, MRB_ARGS_NONE());
}
