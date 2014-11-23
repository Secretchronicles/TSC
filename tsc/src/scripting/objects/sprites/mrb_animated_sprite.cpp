/***************************************************************************
 * mrb_animated_sprite.cpp
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

#include "../../scripting.hpp"
#include "mrb_moving_sprite.hpp"
#include "mrb_animated_sprite.hpp"

/**
 * Class: AnimatedSprite
 *
 * Parent: [MovingSprite](movingsprite.html)
 * {: .superclass}
 *
 * Currently there are no objects exposed to the MRuby API
 * that are _AnimatedSprites_ without being an instance
 * of [MovingSprite](movingsprite.html).
 */

using namespace TSC;
using namespace TSC::Scripting;


static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of this class.");
    return self; // Not reached
}

void TSC::Scripting::Init_Animated_Sprite(mrb_state* p_state)
{
    struct RClass* p_rcAnimated_Sprite = mrb_define_class(p_state, "AnimatedSprite", mrb_class_get(p_state, "MovingSprite"));

    mrb_define_method(p_state, p_rcAnimated_Sprite, "initialize", Initialize, MRB_ARGS_NONE());
}
