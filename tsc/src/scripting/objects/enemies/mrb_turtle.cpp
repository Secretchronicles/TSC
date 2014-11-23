/***************************************************************************
 * mrb_turtle.cpp
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

#include "../../../enemies/turtle.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_turtle.hpp"

/**
 * Class: Turtle
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * Actually, the _Turtles_ do not look like turtles. The name is a
 * relict from previous versions of TSC (<2.0.0) where the turtles
 * were real turtles. Nowadays, the graphic in use is the _Armadillo_,
 * but this means nothing on its semantics. Hit once, it hides in its
 * shell until it thinks danger is over. If you hit the shell, it will
 * start to roll and kill other enemies in its way — or you if you
 * don’t take care.
 *
 * _Note: This class is also accessible via the `Armadillo` constant._
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Turtle::new
 *
 *   new() → a_turtle
 *
 * Creates a new turtle with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cTurtle* p_turtle = new cTurtle(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_turtle;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_turtle->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_turtle);

    return self;
}

/**
 * Method: Turtle#color=
 *
 *   color=( col ) → col
 *
 * Specify the turtle’s color (the default is `:red`).
 *
 * #### Parameters
 * col
 * : The turtle’s new color. One of these symbols: `:red`, `:green`.
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
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid turtle/armadillo color %s", colorstr.c_str());
        return mrb_nil_value(); // Not rached
    }

    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    p_turtle->Set_Color(col);

    return mrb_symbol_value(color);
}

/**
 * Method: Turtle#color
 *
 *   color() → a_symbol
 *
 * Return’s the turtle’s color. See #color= for a list of possible
 * return values.
 */
static mrb_value Get_Color(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    switch (p_turtle->m_color_type) {
    case COL_RED:
        return str2sym(p_state, "red");
    case COL_GREEN:
        return str2sym(p_state, "green");
    default:
        return mrb_nil_value();
    }
}

/**
 * Method: Turtle#walking?
 *
 *   walking?() → true or false
 *
 * Checks whether or not the turtle is walking currently.
 */
static mrb_value Is_Walking(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    return p_turtle->m_turtle_state == TURTLE_WALK ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Turtle#shell_standing?
 *
 *   shell_standing?() → true or false
 *
 * Checks whether or not the turtle is in its shell and not moving.
 */
static mrb_value Is_Shell_Standing(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    return p_turtle->m_turtle_state == TURTLE_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Turtle#shell_moving?
 *
 *   shell_moving?() → true or false
 *
 * Checks whether or not the turtle is in its shell and moving.
 */
static mrb_value Is_Shell_Moving(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    return p_turtle->m_turtle_state == TURTLE_SHELL_RUN ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Turtle#shelled?
 *
 *   shelled?() → true or false
 *
 * Checks whether or not the turtle is in its shell, regardless of
 * whether the shell is moving or not.
 */
static mrb_value Is_Shelled(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    return p_turtle->m_turtle_state == TURTLE_SHELL_RUN || p_turtle->m_turtle_state == TURTLE_SHELL_STAND ? mrb_true_value() : mrb_false_value();
}

/**
 * Method: Turtle#stand_up
 *
 *   stand_up()
 *
 * If the turtle is in its shell, make it come out and walk again.
 * If the turtle is not in its shell, does nothing.
 */
static mrb_value Stand_Up(mrb_state* p_state, mrb_value self)
{
    cTurtle* p_turtle = Get_Data_Ptr<cTurtle>(p_state, self);
    p_turtle->Stand_Up();

    return mrb_nil_value();
}

void TSC::Scripting::Init_Turtle(mrb_state* p_state)
{
    struct RClass* p_rcTurtle = mrb_define_class(p_state, "Turtle", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcTurtle, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcTurtle, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "color=", Set_Color, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcTurtle, "color", Get_Color, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "walking?", Is_Walking, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "shell_standing?", Is_Shell_Standing, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "shell_moving?", Is_Shell_Moving, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "shelled?", Is_Shelled, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcTurtle, "stand_up", Stand_Up, MRB_ARGS_NONE());
}
