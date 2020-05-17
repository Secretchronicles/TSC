/***************************************************************************
 * mrb_armadillo.cpp
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

#include "../../../enemies/army.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "../../events/event.hpp"
#include "mrb_enemy.hpp"
#include "mrb_armadillo.hpp"

/**
 * Class: Armadillo
 *
 * Parent: L<Enemy>
 *
 * Hit once, the I<armadillo> hides in its shell until it thinks danger
 * is over. If you hit the shell, it will start to roll and kill other
 * enemies in its way — or you if you don’t take care.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Downgrade]
 *
 * This event gets triggered when the armadillo gets stumped upon.
 * The event handlers gets passed to arguments, but you should
 * ignore them as they are of no useful value (because an armadillo
 * does not die from stumping upon it).
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(downgrade);

/**
 * Method: Armadillo::new
 *
 *   new() → a_armadillo
 *
 * Creates a new armadillo with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cArmy* p_armadillo = new cArmy(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_armadillo;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_armadillo->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_armadillo);

    return self;
}

/**
 * Method: Armadillo#color=
 *
 *   color=( col ) → col
 *
 * Specify the armadillo’s color (the default is C<:red>).
 *
 * =head4 Parameters
 *
 * =over
 *
 * =item [col]
 *
 * The armadillo’s new color. One of these symbols: C<:red>, C<:green>.
 *
 * =back
 */
static mrb_value Set_Color(mrb_state* p_state, mrb_value self)
{
    mrb_sym color;
    mrb_get_args(p_state, "n", &color);
    std::string colorstr(mrb_sym2name(p_state, color));

    DefaultColor col;
    if (colorstr == "red")
        col = COL_RED;
    else if (colorstr == "green")
        col = COL_GREEN;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid armadillo color %s", colorstr.c_str());
        return mrb_nil_value(); // Not rached
    }

    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    p_armadillo->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Armadillo#color
 *
 *   color() → a_symbol
 *
 * Return’s the armadillo’s color. See #color= for a list of possible
 * return values.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    switch (p_armadillo->m_color_type) {
    case COL_RED:
        return str2sym(p_state, "red");
    case COL_GREEN:
        return str2sym(p_state, "green");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Armadillo#walking?
 *
 *   walking?() → true or false
 *
 * Checks whether or not the armadillo is walking currently.
 */
static mrb_value Is_Walking(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    return p_armadillo->m_army_state == ARMY_WALK ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Armadillo#shell_standing?
 *
 *   shell_standing?() → true or false
 *
 * Checks whether or not the armadillo is in its shell and not moving.
 */
static mrb_value Is_Shell_Standing(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    return p_armadillo->m_army_state == ARMY_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Armadillo#shell_moving?
 *
 *   shell_moving?() → true or false
 *
 * Checks whether or not the armadillo is in its shell and moving.
 */
static mrb_value Is_Shell_Moving(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    return p_armadillo->m_army_state == ARMY_SHELL_RUN ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Armadillo#shelled?
 *
 *   shelled?() → true or false
 *
 * Checks whether or not the armadillo is in its shell, regardless of
 * whether the shell is moving or not.
 */
static mrb_value Is_Shelled(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    return p_armadillo->m_army_state == ARMY_SHELL_RUN || p_armadillo->m_army_state == ARMY_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Armadillo#stand_up
 *
 *   stand_up()
 *
 * If the armadillo is in its shell, make it come out and walk again.
 * If the armadillo is not in its shell, does nothing.
 */
static mrb_value Stand_Up(mrb_state* p_state, mrb_value self)
{
    cArmy* p_armadillo = Get_Data_Ptr<cArmy>(p_state, self);
    p_armadillo->Stand_Up();

    return mrb_nil_value();
}

void TSC::Scripting::Init_Armadillo(mrb_state* p_state)
{
    struct RClass* p_rcArmadillo = mrb_define_class(p_state, "Armadillo", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcArmadillo, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcArmadillo, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "color=", Set_Color, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcArmadillo, "color", Get_Color, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "walking?", Is_Walking, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "shell_standing?", Is_Shell_Standing, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "shell_moving?", Is_Shell_Moving, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "shelled?", Is_Shelled, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcArmadillo, "stand_up", Stand_Up, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcArmadillo, "on_downgrade", MRUBY_EVENT_HANDLER(downgrade), MRB_ARGS_NONE());
}
