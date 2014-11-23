/***************************************************************************
 * mrb_static.cpp
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

#include "../../../enemies/static.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_static.hpp"

/**
 * Class: StaticEnemy
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * _Static enemies_ are usually not-moving enemies that keep their
 * place. The prime example for a static enemy is a saw, which does
 * nothing but rotate around itself.
 *
 * Static enemies don’t change their image by themselves, they always
 * show the same one, just potentially rotated.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: StaticEnemy::new
 *
 *   new() → a_static
 *
 * Creates a new static enemy with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cStaticEnemy* p_static = new cStaticEnemy(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_static;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_static->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_static);

    return self;
}

/**
 * Method: StaticEnemy#rotation_speed=
 *
 *   rotation_speed=( speed ) → speed
 *
 * Specify the velocity the sprite uses to rotate around its centre.
 * By default, static enemies do not rotate (i.e. the speed is 0).
 *
 * #### Parameters
 * speed
 * : The new rotation speed. A float.
 */
static mrb_value Set_Rotation_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    if (speed < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Static enemy rotation speed must be >= 0");
        return mrb_nil_value(); // Not reached
    }

    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    p_static->Set_Rotation_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: StaticEnemy#rotation_speed
 *
 *   rotation_speed() → a_float
 *
 * Returns the static enemy’s rotation speed.
 */
static mrb_value Get_Rotation_Speed(mrb_state* p_state, mrb_value self)
{
    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    return mrb_float_value(p_state, p_static->m_rotation_speed);
}

/**
 * Method: StaticEnemy#speed=
 *
 *   speed=( val ) → val
 *
 * Specify the speed the static enemy uses to move along the TSC
 * path assigned to it. This requires you to assign an TSC path
 * by means of the #path= method.
 *
 * #### Parameters
 * val
 * : The new moving speed of the static enemy.
 */
static mrb_value Set_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    if (speed <= 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Static enemy speed must be >= 0.");
        return mrb_nil_value(); // Not reached
    }

    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    p_static->Set_Speed(speed);

    return mrb_float_value(p_state, speed);
}

/**
 * Method: StaticEnemy#speed
 *
 *   speed() → a_float
 *
 * Returns the static enemy’s moving speed.
 */
static mrb_value Get_Speed(mrb_state* p_state, mrb_value self)
{
    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    return mrb_float_value(p_state, p_static->m_speed);
}

/**
 * Method: StaticEnemy#path=
 *
 *   path=( ident ) → ident
 *
 * Assign a path identifier to this static enemy. If you also
 * set a movement speed using #speed=, this will make the
 * static enemy move along the given path with the specified
 * velocity.
 *
 * #### Parameters
 * path
 * : The identifier of the TSC path you want to assign to
 *   this static enemy, as a string.
 */
static mrb_value Set_Path(mrb_state* p_state, mrb_value self)
{
    char* ident = NULL;
    mrb_get_args(p_state, "z", &ident);

    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    p_static->Set_Path_Identifier(ident);

    return mrb_str_new_cstr(p_state, ident);
}

/**
 * Method: StaticEnemy#path
 *
 *   path() → a_string or nil
 *
 * Get the path identifier assigned to this static enemy.
 *
 * #### Return value
 * The path identifier as a string. If no path identifier has
 * been assigned to this static enemy, returns `nil`.
 */
static mrb_value Get_Path(mrb_state* p_state, mrb_value self)
{
    cStaticEnemy* p_static = Get_Data_Ptr<cStaticEnemy>(p_state, self);
    std::string ident = p_static->m_path_state.m_path_identifier;

    if (ident.empty())
        return mrb_nil_value();
    else
        return mrb_str_new_cstr(p_state, ident.c_str());
}

void TSC::Scripting::Init_StaticEnemy(mrb_state* p_state)
{
    struct RClass* p_rcStaticEnemy = mrb_define_class(p_state, "StaticEnemy", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcStaticEnemy, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcStaticEnemy, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcStaticEnemy, "rotation_speed=", Set_Rotation_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcStaticEnemy, "rotation_speed", Get_Rotation_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcStaticEnemy, "speed=", Set_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcStaticEnemy, "speed", Get_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcStaticEnemy, "path=", Set_Path, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcStaticEnemy, "path", Get_Path, MRB_ARGS_NONE());
}
