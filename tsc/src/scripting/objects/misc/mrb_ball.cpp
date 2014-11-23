/***************************************************************************
 * mrb_ball.cpp
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

#include "../../../objects/ball.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "mrb_ball.hpp"

/**
 * Class: Ball
 *
 * Parent: [AnimatedSprite](animatedsprite.html)
 * {: .superclass}
 *
 * This class represents the fire and ice _balls_ Maryo is able
 * to throw. With this class, you can generate these yourself
 * on whatever conditions you want. Overriding the touch event
 * you could even override the default behaviour.
 */

using namespace TSC;
using namespace TSC::Scripting;

/**
 * Method: Ball::new
 *
 *   new( type ) → a_ball
 *
 * Creates a new ball.
 *
 * ==== Parameters
 * type
 * : The type of the ball. Either `:fire` or `:ice`.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_sym type;
    mrb_get_args(p_state, "n", &type);
    std::string typestr(mrb_sym2name(p_state, type));

    cBall* p_ball = new cBall(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_ball;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    ball_effect effect;
    if (typestr == "fire")
        effect = FIREBALL_DEFAULT;
    else if (typestr == "fire_explosion")
        effect = FIREBALL_EXPLOSION;
    else if (typestr == "ice")
        effect = ICEBALL_DEFAULT;
    else if (typestr == "ice_explosion")
        effect = ICEBALL_EXPLOSION;
    else {
        mrb_raisef(p_state, MRB_ARGUMENT_ERROR(p_state), "Invalid ball effect %s", typestr.c_str());
        return mrb_nil_value(); // not reached
    }

    p_ball->Set_Ball_Type(effect);

    p_ball->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_ball);

    return self;
}

/**
 * Method: Ball#ball_type
 *
 *   ball_type() → a_symbol
 *
 * Returns the ball’s type. See ::new for a list of possible return
 * values.
 */
static mrb_value Get_Ball_Type(mrb_state* p_state, mrb_value self)
{
    cBall* p_ball = Get_Data_Ptr<cBall>(p_state, self);

    switch (p_ball->m_ball_type) {
    case FIREBALL_DEFAULT:
        return str2sym(p_state, "fire");
    case FIREBALL_EXPLOSION:
        return str2sym(p_state, "fire_explosion");
    case ICEBALL_DEFAULT:
        return str2sym(p_state, "ice");
    case ICEBALL_EXPLOSION:
        return str2sym(p_state, "ice_explosion");
    default:
        std::cerr << "Unknown ball effect type " << p_ball->m_ball_type << "." << std::endl;
        return mrb_nil_value();
    }
}

/**
 * Method: Ball#destroy_with_sound
 *
 *   destroy_with_sound()
 *
 * Destroys the ball and plays the sound associated to this ball
 * type’s destruction.
 */
static mrb_value Destroy_With_Sound(mrb_state* p_state, mrb_value self)
{
    cBall* p_ball = Get_Data_Ptr<cBall>(p_state, self);
    p_ball->Destroy_Ball(true);
    return mrb_nil_value();
}

void TSC::Scripting::Init_Ball(mrb_state* p_state)
{
    struct RClass* p_rcBall = mrb_define_class(p_state, "Ball", mrb_class_get(p_state, "AnimatedSprite"));
    MRB_SET_INSTANCE_TT(p_rcBall, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcBall, "initialize", Initialize, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcBall, "ball_type", Get_Ball_Type, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcBall, "destroy_with_sound", Destroy_With_Sound, MRB_ARGS_NONE());
}
