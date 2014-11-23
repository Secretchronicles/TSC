/***************************************************************************
 * scripting.cpp - Glue for the scripting implementation
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

#include "scripting.hpp"
#include "../level/level.hpp"
#include "../level/level_player.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/property_helper.hpp"
#include "../core/filesystem/resource_manager.hpp"

#include "objects/mrb_tsc.hpp"
#include "objects/mrb_eventable.hpp"
#include "objects/mrb_uids.hpp"
#include "objects/sprites/mrb_sprite.hpp"
#include "objects/sprites/mrb_moving_sprite.hpp"
#include "objects/sprites/mrb_animated_sprite.hpp"
#include "objects/sprites/mrb_particle_emitter.hpp"
#include "objects/misc/mrb_level.hpp"
#include "objects/misc/mrb_level_player.hpp"
#include "objects/misc/mrb_audio.hpp"
#include "objects/misc/mrb_input.hpp"
#include "objects/misc/mrb_timer.hpp"
#include "objects/misc/mrb_ball.hpp"
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
#include "objects/enemies/mrb_turtle.hpp"
#include "objects/enemies/mrb_turtle_boss.hpp"
#include "objects/enemies/mrb_pip.hpp"
#include "objects/enemies/mrb_larry.hpp"
#include "objects/powerups/mrb_powerup.hpp"
#include "objects/powerups/mrb_mushroom.hpp"
#include "objects/powerups/mrb_fireberry.hpp"
#include "objects/powerups/mrb_moon.hpp"
#include "objects/powerups/mrb_star.hpp"
#include "objects/boxes/mrb_box.hpp"
#include "objects/boxes/mrb_spinbox.hpp"
#include "objects/boxes/mrb_textbox.hpp"
#include "objects/boxes/mrb_bonusbox.hpp"
#include "objects/specials/mrb_level_exit.hpp"
#include "objects/specials/mrb_level_entry.hpp"
#include "objects/specials/mrb_path.hpp"
#include "objects/specials/mrb_lava.hpp"
#include "objects/specials/mrb_enemy_stopper.hpp"
#include "objects/specials/mrb_goldpiece.hpp"
#include "objects/specials/mrb_jumping_goldpiece.hpp"
#include "objects/specials/mrb_falling_goldpiece.hpp"
#include "objects/specials/mrb_crate.hpp"
#include "objects/specials/mrb_moving_platform.hpp"

////////////////////////////////////////
// Be sure to review docs/scripting.md!
////////////////////////////////////////

// Extern
mrb_data_type TSC::Scripting::rtTSC_Scriptable = {"TscScriptable", NULL};

namespace TSC {

namespace Scripting {

cMRuby_Interpreter::cMRuby_Interpreter(cLevel* p_level)
{
    // Set member variables
    mp_level = p_level;
    mp_mruby = mrb_open();

    // Load TSC classes into mruby
    Load_Wrappers();
    // Load scripting library
    Load_Scripts();
}

cMRuby_Interpreter::~cMRuby_Interpreter()
{
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

    // Terminate mruby interpreter
    mrb_close(mp_mruby);
}

mrb_state* cMRuby_Interpreter::Get_MRuby_State()
{
    return mp_mruby;
}

cLevel* cMRuby_Interpreter::Get_Level()
{
    return mp_level;
}

mrb_value cMRuby_Interpreter::Run_Code_In_Context(const std::string& code, mrbc_context* p_context)
{
    return mrb_load_nstring_cxt(mp_mruby, code.c_str(), code.length(), p_context);
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
        mrb_print_error(mp_mruby);
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
        std::cerr << "Failed to open mruby script file '" << path_to_utf8(filepath) << "'" << std::endl;
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
    // Load the main scripting file. This file is supposed to
    // do any custom user scripting startup stuff.
    boost::filesystem::path mainfile = pResource_Manager->Get_Game_Scripting("main.rb");

    // Warn user if user’s main.rb errors.
    if (!Run_File(mainfile)) {
        std::cerr << "Warning: Error loading main mruby script '"
                  << path_to_utf8(mainfile)
                  << "'!" << std::endl;
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
            std::cerr << "Warning: Error running timer callback: " << std::endl;
            mrb_print_error(mp_mruby);
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
    Init_Animated_Sprite(mp_mruby);
    Init_Level(mp_mruby);
    Init_Level_Player(mp_mruby);
    Init_Input(mp_mruby);
    Init_Audio(mp_mruby);
    Init_Timer(mp_mruby);
    Init_Ball(mp_mruby);
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
    Init_Turtle(mp_mruby);
    Init_TurtleBoss(mp_mruby);
    Init_Larry(mp_mruby);
    Init_Powerup(mp_mruby);
    Init_Mushroom(mp_mruby);
    Init_Fireberry(mp_mruby);
    Init_Moon(mp_mruby);
    Init_Star(mp_mruby);
    Init_Box(mp_mruby);
    Init_SpinBox(mp_mruby);
    Init_TextBox(mp_mruby);
    Init_BonusBox(mp_mruby);
    Init_ParticleEmitter(mp_mruby);
    Init_LevelExit(mp_mruby);
    Init_LevelEntry(mp_mruby);
    Init_Path(mp_mruby);
    Init_Lava(mp_mruby);
    Init_EnemyStopper(mp_mruby);
    Init_Goldpiece(mp_mruby);
    Init_JumpingGoldpiece(mp_mruby);
    Init_FallingGoldpiece(mp_mruby);
    Init_Crate(mp_mruby);
    Init_Moving_Platform(mp_mruby);
    Init_UIDS(mp_mruby); // Call this last so it can rely on the other MRuby classes to be defined
}
}
}
