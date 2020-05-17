/***************************************************************************
 * scripting.cpp - Glue for the scripting implementation
 *
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "scripting.hpp"
#include "../level/level.hpp"
#include "../level/level_player.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/property_helper.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../core/i18n.hpp"
#include "../audio/audio.hpp"
#include "../user/savegame/savegame.hpp"
#include "../input/keyboard.hpp"
#include "../gui/game_console.hpp"

#include "objects/mrb_tsc.hpp"
#include "objects/mrb_eventable.hpp"
#include "objects/mrb_uids.hpp"
#include "objects/sprites/mrb_sprite.hpp"
#include "objects/sprites/mrb_moving_sprite.hpp"
#include "objects/sprites/mrb_particle_emitter.hpp"
#include "objects/misc/mrb_level.hpp"
#include "objects/misc/mrb_level_player.hpp"
#include "objects/misc/mrb_audio.hpp"
#include "objects/misc/mrb_input.hpp"
#include "objects/misc/mrb_timer.hpp"
#include "objects/misc/mrb_ball.hpp"
#include "objects/misc/mrb_scene.hpp"
#include "objects/misc/mrb_cinematic_scene.hpp"
#include "objects/enemies/mrb_enemy.hpp"
#include "objects/enemies/mrb_beetle.hpp"
#include "objects/enemies/mrb_beetle_barrage.hpp"
#include "objects/enemies/mrb_eato.hpp"
#include "objects/enemies/mrb_flyon.hpp"
#include "objects/enemies/mrb_furball.hpp"
#include "objects/enemies/mrb_gee.hpp"
#include "objects/enemies/mrb_krush.hpp"
#include "objects/enemies/mrb_rokko.hpp"
#include "objects/enemies/mrb_spika.hpp"
#include "objects/enemies/mrb_spikeball.hpp"
#include "objects/enemies/mrb_static.hpp"
#include "objects/enemies/mrb_thromp.hpp"
#include "objects/enemies/mrb_armadillo.hpp"
#include "objects/enemies/mrb_turtle_boss.hpp"
#include "objects/enemies/mrb_pip.hpp"
#include "objects/enemies/mrb_larry.hpp"
#include "objects/enemies/mrb_doom_larry.hpp"
#include "objects/powerups/mrb_powerup.hpp"
#include "objects/powerups/mrb_berry.hpp"
#include "objects/powerups/mrb_fireberry.hpp"
#include "objects/powerups/mrb_cookie.hpp"
#include "objects/powerups/mrb_lemon.hpp"
#include "objects/boxes/mrb_box.hpp"
#include "objects/boxes/mrb_spinbox.hpp"
#include "objects/boxes/mrb_textbox.hpp"
#include "objects/boxes/mrb_bonusbox.hpp"
#include "objects/specials/mrb_level_exit.hpp"
#include "objects/specials/mrb_level_entry.hpp"
#include "objects/specials/mrb_secret_area.hpp"
#include "objects/specials/mrb_path.hpp"
#include "objects/specials/mrb_lava.hpp"
#include "objects/specials/mrb_enemy_stopper.hpp"
#include "objects/specials/mrb_jewel.hpp"
#include "objects/specials/mrb_jumping_jewel.hpp"
#include "objects/specials/mrb_falling_jewel.hpp"
#include "objects/specials/mrb_crate.hpp"
#include "objects/specials/mrb_moving_platform.hpp"
#include "../core/global_basic.hpp"

////////////////////////////////////////
// Be sure to review docs/scripting.md!
////////////////////////////////////////

// Extern
mrb_data_type TSC::Scripting::rtTSC_Scriptable = {"TscScriptable", NULL};

using namespace std;

namespace TSC {

namespace Scripting {

cMRuby_Interpreter::cMRuby_Interpreter(cLevel* p_level)
{
    // Set member variables
    mp_level = p_level;
    mp_mruby = mrb_open();

    // Create console context (execution context for the game console)
    mp_console_ctx = mrbc_context_new(mp_mruby);
    mp_console_ctx->lineno = 1;
    // TRANS: Prompt issued in the game console
    mrbc_filename(mp_mruby, mp_console_ctx, _("(console)"));

    // Load TSC classes into mruby
    Load_Wrappers();
    // Load scripting library
    Load_Scripts();
}

cMRuby_Interpreter::~cMRuby_Interpreter()
{
    /* When the mruby interpreter gets deleted, all remaining mruby objects
     * (mrb_value instances) are invalidated. Therefore, we wipe all the
     * existing event callbacks here. */
    std::string levelname = path_to_utf8(pActive_Level->m_level_filename.stem());
    cSprite_List::iterator iter;
    for (iter = mp_level->m_sprite_manager->objects.begin(); iter != mp_level->m_sprite_manager->objects.end(); iter++) {
        cSprite* p_sprite = *iter;
        p_sprite->clear_event_handlers(levelname); // Would probably work fine without the level name (→ total clearing) as these sprites do not live longer than the level itself anyway
    }
    // These objects stay alive even though a level ends. Only wipe those
    // handlers for our own level.
    pAudio->clear_event_handlers(levelname);
    pKeyboard->clear_event_handlers(levelname);
    pSavegame->clear_event_handlers(levelname);
    pLevel_Player->clear_event_handlers(levelname);

    // Get all the registered timers from mruby
    mrb_value klass = mrb_obj_value(mrb_class_get(mp_mruby, "Timer"));
    mrb_value rb_timers = mrb_iv_get(mp_mruby, klass, mrb_intern_cstr(mp_mruby, "instances"));

    // Stop ’em all! (and free them)
    while (true) {
        // Retrieve timer
        mrb_value rb_timer = mrb_ary_shift(mp_mruby, rb_timers);
        if (mrb_nil_p(rb_timer))
            break;

        // Free C++ part. The mruby part is out of scope now (shifted from
        // the instance array) and will be GC’ed (would anyway due to termination
        // further below). Note cTimer’s destructor calls Interrupt() on the timer.
        cTimer* p_timer = Get_Data_Ptr<cTimer>(mp_mruby, rb_timer);
        delete p_timer;
    }

    // Release console context
    mrbc_context_free(mp_mruby, mp_console_ctx);

    // Terminate mruby interpreter
    mrb_close(mp_mruby);
}

mrb_state* cMRuby_Interpreter::Get_MRuby_State()
{
    return mp_mruby;
}

const mrbc_context* cMRuby_Interpreter::Get_Console_Context() const
{
    return mp_console_ctx;
}

cLevel* cMRuby_Interpreter::Get_Level()
{
    return mp_level;
}

mrb_value cMRuby_Interpreter::Run_Code_In_Context(const std::string& code, mrbc_context* p_context)
{
    mrb_value retval = mrb_load_nstring_cxt(mp_mruby, code.c_str(), code.length(), p_context);
    int newlines = std::count(code.begin(), code.end(), '\n');
    p_context->lineno += newlines;
    return retval;
}

mrb_value cMRuby_Interpreter::Run_Code_In_Console_Context(const std::string& code)
{
    return Run_Code_In_Context(code, mp_console_ctx);
}

bool cMRuby_Interpreter::Run_Code(const std::string& code, const std::string& contextname)
{
    // Create a new context. This is important so we
    // can properly retrieve exceptions, which mrb_load_string()
    // does not allow.
    mrbc_context* p_context = mrbc_context_new(mp_mruby);
    p_context->capture_errors = true;
    p_context->lineno = 1;
    mrbc_filename(mp_mruby, p_context, contextname.c_str()); // Set context filename (for exceptions)

    Run_Code_In_Context(code, p_context);

    bool result;
    if (mp_mruby->exc) {
        // Exception occured
        gp_game_console->Display_Exception(mp_mruby);

        // Clear exception pointer so execution can continue
        mp_mruby->exc = NULL;
        result = false;
    }
    else
        result = true;

    mrbc_context_free(mp_mruby, p_context);
    return result;
}

bool cMRuby_Interpreter::Run_File(const boost::filesystem::path& filepath)
{
    // Note we cannot use mrb_load_file(), because we use boost::filesystem’s
    // filereading capabilities which mruby doesn’t understand. Instead, we
    // simply pass the read file’s contents to mruby.

    // Open the file.
    boost::filesystem::ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Failed to open mruby script file '" << path_to_utf8(filepath) << "'" << endl;
        return false;
    }

    // Read it.
    std::string code = readfile(file);
    file.close();

    // Compile & execute it.
    return Run_Code(code, path_to_utf8(filepath.filename()).c_str());
}

void cMRuby_Interpreter::Load_Scripts()
{
    // Load the SSL (Standard Scripting Library)
    std::vector<boost::filesystem::path> scriptfiles;

    for (boost::filesystem::directory_iterator diter(pResource_Manager->Get_Game_Scripting_Directory()); diter != boost::filesystem::directory_iterator(); diter++) {
        if (diter->path().extension() == utf8_to_path(".rb")) {
            scriptfiles.push_back(diter->path());
        }
    }

    debug_print("Scripting engine: loading %ld script files\n", scriptfiles.size());
    std::sort(scriptfiles.begin(), scriptfiles.end());

    for (boost::filesystem::path scriptfile: scriptfiles) {
        debug_print("Scripting engine: loading script file '%s'\n", path_to_utf8(scriptfile).c_str());
        if (!Run_File(scriptfile)) {
            std::cerr << "Scripting engine: warning: error loading mruby script file '"
                      << path_to_utf8(scriptfile)
                      << "'!" << std::endl;
        }
    }

    // Load user's scripting expansion packs
    std::vector<boost::filesystem::path> user_script_dirs;
    for (boost::filesystem::directory_iterator diter(pResource_Manager->Get_User_Scripting_Directory()); diter != boost::filesystem::directory_iterator(); diter++) {
        if (boost::filesystem::is_directory(diter->path())) {
            user_script_dirs.push_back(diter->path());
        }
    }
    std::sort(user_script_dirs.begin(), user_script_dirs.end());
    debug_print("Scripting engine: loading %ld user script expansion packs\n", user_script_dirs.size());

    for (const boost::filesystem::path& scriptdir: user_script_dirs) {
        scriptfiles.clear();
        for (boost::filesystem::directory_iterator diter(scriptdir); diter != boost::filesystem::directory_iterator(); diter++) {
            if (diter->path().extension() == utf8_to_path(".rb")) {
                scriptfiles.push_back(diter->path());
            }

            debug_print("Scripting engine: loading %ld script files from user script expansion pack %s\n", scriptfiles.size(), path_to_utf8(scriptdir.filename()).c_str());
            std::sort(scriptfiles.begin(), scriptfiles.end());

            for (boost::filesystem::path scriptfile: scriptfiles) {
                debug_print("Scripting engine: loading script file '%s'\n", path_to_utf8(scriptfile).c_str());
                if (!Run_File(scriptfile)) {
                    std::cerr << "Scripting engine: warning: error loading mruby script file '"
                              << path_to_utf8(scriptfile)
                              << "'!" << std::endl;
                }
            }
        }
    }

}

void cMRuby_Interpreter::Register_Callback(mrb_value callback)
{
    // Note we need to lock the access to the list of callbacks
    // to prevent race conditions.
    boost::lock_guard<boost::mutex> _lock(m_callback_mutex);
    m_callbacks.push_back(callback);
}

void cMRuby_Interpreter::Evaluate_Timer_Callbacks()
{
    // Note we need to lock the access to the list of callbacks
    // to prevent race conditions.
    boost::lock_guard<boost::mutex> _lock(m_callback_mutex);

    // Don’t put unnecessary strain in the mainloop (this method
    // is called once a frame!) if no timers are there.
    if (m_callbacks.empty())
        return;

    // Iterate through the list of registered callbacks
    // and evaluate each one
    std::vector<mrb_value>::iterator iter;
    for (iter = m_callbacks.begin(); iter != m_callbacks.end(); iter++) {
        mrb_funcall(mp_mruby, *iter, "call", 0);
        if (mp_mruby->exc) {
            // Exception occured
            gp_game_console->Display_Exception(mp_mruby);
            mp_mruby->exc = NULL;
        }
    }

    // Empty the list of registered callbacks. The timers
    // will add  to it again when necessary.
    m_callbacks.clear();
}

/**
 * Adds `obj' to an internal array that is referenced from
 * the existing TSC mruby module so that the object is
 * considered referenced by the mruby intepreter and does
 * not get garbage-collected. Always protect an object that
 * has been created internally and has no references to it
 * with this method before you use it in function calls.
 *
 * Returns the index at which the object was stored.
 */
mrb_int cMRuby_Interpreter::Protect_From_GC(mrb_value obj)
{
    mrb_value mod_tsc = mrb_const_get(mp_mruby, mrb_obj_value(mp_mruby->object_class), mrb_intern_cstr(mp_mruby, "TSC"));
    mrb_value hsh     = mrb_iv_get(mp_mruby, mod_tsc, mrb_intern_cstr(mp_mruby, "gc_protector"));
    mrb_int   oid     = mrb_obj_id(obj);

    // TODO: Is the object ID request really secure from GC?
    mrb_hash_set(mp_mruby, hsh, mrb_fixnum_value(oid), obj);
    return oid;
}

/**
 * Counterpart to Protect_From_GC(). Pass the return value
 * you received from that method to this method in order
 * to release the object so that the Garbage Collector can
 * free the object.
 */
void cMRuby_Interpreter::Unprotect_From_GC(mrb_int index)
{
    mrb_value mod_tsc = mrb_const_get(mp_mruby, mrb_obj_value(mp_mruby->object_class), mrb_intern_cstr(mp_mruby, "TSC"));
    mrb_value hsh     = mrb_iv_get(mp_mruby, mod_tsc, mrb_intern_cstr(mp_mruby, "gc_protector"));

    mrb_hash_delete_key(mp_mruby, hsh, mrb_fixnum_value(index));
}

void cMRuby_Interpreter::Load_Wrappers()
{
    using namespace TSC::Scripting;

    // Create the main TSC modules
    Init_TSC(mp_mruby);
    Init_Eventable(mp_mruby);

    // Create a variable that can be used specifically for
    // referencing internal mruby objects so they don’t
    // get collected by mruby’s Garbage Collector (GC)
    mrb_value mod_tsc = mrb_const_get(mp_mruby, mrb_obj_value(mp_mruby->object_class), mrb_intern_cstr(mp_mruby, "TSC"));
    mrb_iv_set(mp_mruby, mod_tsc, mrb_intern_cstr(mp_mruby, "gc_protector"), mrb_hash_new(mp_mruby));

    // When changing the order, ensure parent mruby classes get defined
    // prior to their mruby subclasses!
    Init_Sprite(mp_mruby);
    Init_Moving_Sprite(mp_mruby);
    Init_Level(mp_mruby);
    Init_Level_Player(mp_mruby);
    Init_Input(mp_mruby);
    Init_Audio(mp_mruby);
    Init_Timer(mp_mruby);
    Init_Ball(mp_mruby);
    Init_Scene(mp_mruby);
    Init_Cinematic_Scene(mp_mruby);
    Init_Enemy(mp_mruby);
    Init_Beetle(mp_mruby);
    Init_BeetleBarrage(mp_mruby);
    Init_Eato(mp_mruby);
    Init_Flyon(mp_mruby);
    Init_Furball(mp_mruby);
    Init_Gee(mp_mruby);
    Init_Krush(mp_mruby);
    Init_Pip(mp_mruby);
    Init_Rokko(mp_mruby);
    Init_Spika(mp_mruby);
    Init_Spikeball(mp_mruby);
    Init_StaticEnemy(mp_mruby);
    Init_Thromp(mp_mruby);
    Init_Armadillo(mp_mruby);
    Init_TurtleBoss(mp_mruby);
    Init_Larry(mp_mruby);
    Init_DoomLarry(mp_mruby);
    Init_Powerup(mp_mruby);
    Init_Berry(mp_mruby);
    Init_Fireberry(mp_mruby);
    Init_Cookie(mp_mruby);
    Init_Lemon(mp_mruby);
    Init_Box(mp_mruby);
    Init_SpinBox(mp_mruby);
    Init_TextBox(mp_mruby);
    Init_BonusBox(mp_mruby);
    Init_ParticleEmitter(mp_mruby);
    Init_LevelExit(mp_mruby);
    Init_LevelEntry(mp_mruby);
    Init_SecretArea(mp_mruby);
    Init_Path(mp_mruby);
    Init_Lava(mp_mruby);
    Init_EnemyStopper(mp_mruby);
    Init_Jewel(mp_mruby);
    Init_JumpingJewel(mp_mruby);
    Init_FallingJewel(mp_mruby);
    Init_Crate(mp_mruby);
    Init_Moving_Platform(mp_mruby);
    Init_UIDS(mp_mruby); // Call this last so it can rely on the other MRuby classes to be defined
}
}
}
