/***************************************************************************
 * mrb_beetle.cpp
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

#include "../../../enemies/beetle.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_beetle.hpp"

/**
 * Class: Beetle
 *
 * Parent: L<Enemy>
 *
 * I<Beetles> are small bugs that are usually spit out from
 * a L<Beetle Barrage|BeetleBarrage>. Using this class,
 * you can do with them whatever you want.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Beetle::new
 *
 *   new() → a_beetle
 *
 * Creates a new instance of this class.
 *
 * =head4 Return value
 *
 * A new instance.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cBeetle* p_beetle = new cBeetle(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_beetle;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_beetle->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_beetle);

    return self;
}

/**
 * Method: Beetle#rest_living_time=
 *
 *   rest_living_time=( time )
 *
 * TODO: Docs.
 */
static mrb_value Set_Rest_Living_Time(mrb_state* p_state, mrb_value self)
{
    float time = 0.0f;
    mrb_get_args(p_state, "f", &time);

    cBeetle* p_beetle = Get_Data_Ptr<cBeetle>(p_state, self);
    p_beetle->Set_Rest_Living_Time(time);

    return mrb_float_value(p_state, time);
}

/**
 * Method: Beetle#rest_living_time
 *
 *   rest_living_time() → a_float
 *
 * TODO: Docs.
 */
static mrb_value Get_Rest_Living_Time(mrb_state* p_state, mrb_value self)
{
    cBeetle* p_beetle = Get_Data_Ptr<cBeetle>(p_state, self);
    return mrb_float_value(p_state, p_beetle->Get_Rest_Living_Time());
}

/**
 * Method: Beetle#color=
 *
 *   color=( color ) → color
 *
 * Sets the color for the beetle.
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [color]
 *
 * The beetle’s new color. One of the following symbols:
 * C<:blue>, C<:green>, C<:red>, C<:violet>, C<:yellow>.
 *
 * =back
 */
static mrb_value Set_Color(mrb_state* p_state, mrb_value self)
{
    mrb_sym color;
    mrb_get_args(p_state, "n", &color);
    std::string colorstr = mrb_sym2name(p_state, color);

    DefaultColor col;
    if (colorstr == "blue")
        col = COL_BLUE;
    else if (colorstr == "green")
        col = COL_GREEN;
    else if (colorstr == "red")
        col = COL_RED;
    else if (colorstr == "violet")
        col = COL_VIOLET;
    else if (colorstr == "yellow")
        col = COL_YELLOW;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid beetle color %s", colorstr.c_str());
        return mrb_nil_value(); // Not reached
    }

    cBeetle* p_beetle = Get_Data_Ptr<cBeetle>(p_state, self);
    p_beetle->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Beetle#color
 *
 *   color() → a_symbol
 *
 * Returns the beetle’s current color as a symbol. See L<#color=> for
 * a list of possible return values.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cBeetle* p_beetle = Get_Data_Ptr<cBeetle>(p_state, self);

    switch (p_beetle->Get_Color()) {
    case COL_BLUE:
        return str2sym(p_state, "blue");
    case COL_GREEN:
        return str2sym(p_state, "green");
    case COL_RED:
        return str2sym(p_state, "red");
    case COL_VIOLET:
        return str2sym(p_state, "violet");
    case COL_YELLOW:
        return str2sym(p_state, "yellow");
    default:
        return mrb_nil_value();
    }
}

void TSC::Scripting::Init_Beetle(mrb_state* p_state)
{
    struct RClass* p_rcBeetle = mrb_define_class(p_state, "Beetle", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcBeetle, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcBeetle, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetle, "rest_living_time", Get_Rest_Living_Time, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetle, "rest_living_time=", Set_Rest_Living_Time, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBeetle, "color", Get_Color, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBeetle, "color=", Set_Color, MRB_ARGS_REQ(1));
}
