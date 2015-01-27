/***************************************************************************
 * mrb_enemy.cpp
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

#include "mrb_enemy.hpp"
#include "../sprites/mrb_moving_sprite.hpp"
#include "../../../enemies/enemy.hpp"
#include "../../../gui/hud.hpp"
#include "../../events/event.hpp"

/**
 * Class: Enemy
 *
 * Parent: [MovingSprite](movingsprite.html)
 * {: .superclass}
 *
 * _Enemies_ are the little guys that hang around all over the world of
 * TSC and try to hinder you in many different ways. This is the base
 * class for all enemies and defines the methods that are available for
 * every enemy, regardless of its type.
 *
 * Currently, this class is not instanciatable.
 *
 * Events
 * ------
 *
 * Die
 * : This event gets triggered when the enemy dies. The event handler
 *   doesn’t get passed any argument.
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(die);


static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of this class.");
    return self; // Not reached
}

/**
 * Method: Enemy#kill!
 *
 *   kill!()
 *
 * Immediately kills this enemy. No points are given to
 * the player (i.e. it is as if the enemy fell into an abyss
 * or something like that).
 *
 * Causes a subsequent _Die_ event.
 *
 * See also: [#kill_with_points!](#killwithpoints).
 */
static mrb_value Kill(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);

    p_enemy->Set_Dead(true);
    return mrb_nil_value();
}

/**
 * Method: Enemy#kill_with_points!
 *
 *   kill_with_points!()
 *
 * Immediately kills this enemy. Points are given to the
 * player. Also plays the dying sound for this enemy.
 *
 * Causes a subsequent _Die_ event.
 *
 * See also: [#kill!](#kill)
 */
static mrb_value Kill_With_Points(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);

    pHud_Points->Add_Points(p_enemy->m_kill_points,
                            p_enemy->m_pos_x,
                            p_enemy->m_pos_y - 5.0f);
    pAudio->Play_Sound(p_enemy->m_kill_sound);
    p_enemy->Set_Dead(true);

    return mrb_nil_value();
}

/**
 * Method: Enemy#kill_points
 *
 *   kill_points() → an_integer
 *
 * Returns the number of points the player gains after killing this
 * enemy.
 */
static mrb_value Get_Kill_Points(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);

    return mrb_fixnum_value(p_enemy->m_kill_points);
}

/**
 * Method: Enemy#kill_points=
 *
 *   kill_points=( points )
 *
 * Sets the number of points the player gains after killing this
 * enemy. Note kill points are not applied if the enemy dies due to
 * something else than player interaction, e.g. by falling into an
 * abyss.
 *
 * #### Parameters
 * points
 * : Number of points to add.
*/
static mrb_value Set_Kill_Points(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);
    mrb_int points;
    mrb_get_args(p_state, "i", &points);
    p_enemy->m_kill_points = points;

    return mrb_fixnum_value(points);
}

/**
 * Method: Enemy#kill_sound
 *
 *   kill_sound() → a_string
 *
 * Returns the filename of the sound to play when the enemy gets killed,
 * relative to the `sounds/` directory.
 */
static mrb_value Get_Kill_Sound(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);

    return mrb_str_new_cstr(p_state, p_enemy->m_kill_sound.c_str());
}

/**
 * Method: Enemy#kill_sound=
 *
 *   kill_sound=( path )
 *
 * Sets the sound to play when the enemy gets killed.
 *
 * #### Parameters
 * path
 * : The path of the sound file to play. Relative to the `sounds/`
 *   directory.
*/
static mrb_value Set_Kill_Sound(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);
    char* path;
    mrb_get_args(p_state, "z", &path);
    p_enemy->m_kill_sound = path;

    return mrb_str_new_cstr(p_state, path);
}

/**
 * Method: Enemy#fire_resistant=
 *
 *   fire_resistant=( val )
 *
 * Enables/Disables fire resistance for this enemy.
 *
 * #### Parameters
 * val
 * : `true` to enable the fire resistance, `false` to disable it.
*/
static mrb_value Set_Fire_Resistant(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);
    mrb_value obj;
    mrb_get_args(p_state, "o", &obj);
    p_enemy->m_fire_resistant = mrb_test(obj) ? true : false;

    return obj;
}

/**
 * Method: Enemy#fire_resistant?
 *
 *   fire_resistant?
 *
 * Checks whether this enemy is resistant to fire.
 */
static mrb_value Is_Fire_Resistant(mrb_state* p_state,  mrb_value self)
{
    cEnemy* p_enemy = Get_Data_Ptr<cEnemy>(p_state, self);

    return p_enemy->m_fire_resistant ? mrb_true_value() : mrb_false_value();
}


void TSC::Scripting::Init_Enemy(mrb_state* p_state)
{
    struct RClass* p_rcEnemy = mrb_define_class(p_state, "Enemy", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcEnemy, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcEnemy, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEnemy, "kill_points", Get_Kill_Points, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEnemy, "kill_points=", Set_Kill_Points, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcEnemy, "kill_sound", Get_Kill_Sound, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEnemy, "kill_sound=", Set_Kill_Sound, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcEnemy, "fire_resistant=", Set_Fire_Resistant, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcEnemy, "fire_resistant?", Is_Fire_Resistant, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEnemy, "kill!", Kill, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcEnemy, "kill_with_points!", Kill_With_Points, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcEnemy, "on_die", MRUBY_EVENT_HANDLER(die), MRB_ARGS_NONE());
}
