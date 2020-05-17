/***************************************************************************
 * mrb_gee.cpp
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

#include "../../../enemies/gee.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_gee.hpp"

/**
 * Class: Gee
 *
 * Parent: L<Enemy>
 *
 * The I<Gee> is a flying monster that moves slowly or quickly along
 * a fixed path. Red gees are immunate to fire attacks.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Gee::new
 *
 *   new() → a_gee
 *
 * Create a new gee with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cGee* p_gee = new cGee(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_gee;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_gee->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_gee);

    return self;
}

/**
 * Method: Gee#color=
 *
 *   color=( color ) → color
 *
 * Set this gee’s color. Note that changing color also resets some
 * attributes depending on the color (i.e. if you make a gee C<:red>,
 * it will automatically gain fire resistance, which you could then
 * undo by calling L<#fire_resistance=> on it afterwards).
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [color]
 *
 * One of the following, self-explanatory symbols:
 * C<:red>, C<:green>, C<:blue>.
 *
 * =back
 */
static mrb_value Set_Color(mrb_state* p_state, mrb_value self)
{
    mrb_sym color;
    mrb_get_args(p_state, "n", &color);
    std::string colorstr(mrb_sym2name(p_state, color));

    DefaultColor col;
    if (colorstr == "yellow")
        col = COL_YELLOW;
    else if (colorstr == "red")
        col = COL_RED;
    else if (colorstr == "green")
        col = COL_GREEN;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid Gee color %s", colorstr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    p_gee->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Gee#color
 *
 *   color() → a_symbol
 *
 * Retrieve the gee’s color.
 *
 * =head4 Return value
 *
 * The gee’s current color as a symbol. See L<#color=> for
 * possible values.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);

    switch (p_gee->m_color_type) {
    case COL_YELLOW:
        return str2sym(p_state, "yellow");
    case COL_RED:
        return str2sym(p_state, "red");
    case COL_GREEN:
        return str2sym(p_state, "green");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Gee#max_distance=
 *
 *   max_distance=( distance ) → distance
 *
 * Set the maximum distance the gee may move.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [distance] The maximum distance to travel, in pixels.
 *
 * =back
 */
static mrb_value Set_Max_Distance(mrb_state* p_state, mrb_value self)
{
    mrb_int maxdistance;
    mrb_get_args(p_state, "i", &maxdistance);

    if (maxdistance < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Gee max distance must be > 0.");
        return mrb_nil_value(); // Not reached
    }

    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    p_gee->Set_Max_Distance(maxdistance);

    return mrb_fixnum_value(maxdistance);
}

/**
 * Method: Gee#max_distance
 *
 *   max_distance() → an_integer
 *
 * The maximum distance the gee may travel, in pixels.
 */
static mrb_value Get_Max_Distance(mrb_state* p_state, mrb_value self)
{
    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    return mrb_fixnum_value(p_gee->m_max_distance);
}

/**
 * Method: Gee#fly_speed=
 *
 *   fly_speed=( speed ) → speed
 *
 * Set the gee’s flying velocity.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [speed] The new flying velocity. A float.
 *
 * =back
 */
static mrb_value Set_Fly_Speed(mrb_state* p_state, mrb_value self)
{
    mrb_float speed;
    mrb_get_args(p_state, "f", &speed);

    if (speed < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Gee fly speed must be > 0.");
        return mrb_nil_value(); // Not reached
    }

    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    p_gee->m_speed_fly = speed;

    return mrb_float_value(p_state, speed);
}

/**
 * Method: Gee#fly_speed
 *
 *   fly_speed() → a_float
 *
 * Returns the gee’s flying velocity.
 */
static mrb_value Get_Fly_Speed(mrb_state* p_state, mrb_value self)
{
    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    return mrb_float_value(p_state, p_gee->m_speed_fly);
}

/**
 * Method: Gee#always_flying=
 *
 *   always_flying( bool ) → bool
 *
 * Set to C<true> to have this gee never wait.
 */
static mrb_value Set_Always_Fly(mrb_state* p_state, mrb_value self)
{
    mrb_bool fly;
    mrb_get_args(p_state, "b", &fly);

    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    p_gee->m_always_fly = fly;

    return mrb_bool_value(fly);
}

/**
 * Method: Gee#always_flying?
 *
 *   always_flying?() → true or false
 *
 * Check whether this gee is always moving.
 */
static mrb_value Does_Always_Fly(mrb_state* p_state, mrb_value self)
{
    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    return p_gee->m_always_fly ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Gee#wait_time=
 *
 *   wait_time=( seconds ) → seconds
 *
 * Specify how long to wait between movements. Note this value is
 * ignored if L<#always_flying> is C<true>.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [seconds]
 *
 * New number of seconds to wait until next movement. A float.
 *
 * =back
 */
static mrb_value Set_Wait_Time(mrb_state* p_state, mrb_value self)
{
    mrb_float time;
    mrb_get_args(p_state, "f", &time);

    if (time < 0) {
        mrb_raise(p_state, MRB_RANGE_ERROR(p_state), "Gee wait time must be > 0.");
        return mrb_nil_value(); // Not reached
    }

    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    p_gee->m_wait_time = time;

    return mrb_float_value(p_state, time);
}

/**
 * Method: Gee#wait_time
 *
 *   wait_time() → a_float
 *
 * Returns the number of seconds the gee waits between movements
 * as a float. Note this value is ignored if `always_flying?` is
 * `true`.
 */
static mrb_value Get_Wait_Time(mrb_state* p_state, mrb_value self)
{
    cGee* p_gee = Get_Data_Ptr<cGee>(p_state, self);
    return mrb_float_value(p_state, p_gee->m_wait_time);
}

void TSC::Scripting::Init_Gee(mrb_state* p_state)
{
    struct RClass* p_rcGee = mrb_define_class(p_state, "Gee", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcGee, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcGee, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcGee, "color=", Set_Color, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcGee, "color", Get_Color, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcGee, "max_distance=", Set_Max_Distance, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcGee, "max_distance", Get_Max_Distance, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcGee, "fly_speed=", Set_Fly_Speed, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcGee, "fly_speed", Get_Fly_Speed, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcGee, "always_flying=", Set_Always_Fly, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcGee, "always_flying?", Does_Always_Fly, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcGee, "wait_time=", Set_Wait_Time, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcGee, "wait_time", Get_Wait_Time, MRB_ARGS_NONE());
}
