/***************************************************************************
 * mrb_falling_goldpiece.cpp
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

#include "../../../objects/goldpiece.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_jumping_goldpiece.hpp"
#include "mrb_goldpiece.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/**
 * Class: FallingGoldpiece
 *
 * Parent: [Goldpiece](goldpiece.html)
 * {: .superclass}
 *
 * This kind of goldpiece jumps around when it falls down
 * something, i.e. it is subject to gravity and moves around.
 * This is goldpiece type used when you eliminate an enemy
 * by means of a fireball.
 */


/**
 * Method: FallingGoldpiece::new
 *
 *   new() → a_falling_goldpiece
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cJGoldpiece* p_fgp = new cJGoldpiece(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_fgp;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_fgp->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_fgp);

    return self;
}

void TSC::Scripting::Init_FallingGoldpiece(mrb_state* p_state)
{
    struct RClass* p_rcFalling_Goldpiece = mrb_define_class(p_state, "FallingGoldpiece", mrb_class_get(p_state, "Goldpiece"));
    MRB_SET_INSTANCE_TT(p_rcFalling_Goldpiece, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcFalling_Goldpiece, "initialize", Initialize, MRB_ARGS_NONE());
}
