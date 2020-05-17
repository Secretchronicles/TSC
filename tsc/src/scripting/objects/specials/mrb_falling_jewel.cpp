/***************************************************************************
 * mrb_falling_jewel.cpp
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

#include "../../../objects/goldpiece.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_jumping_jewel.hpp"
#include "mrb_jewel.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/**
 * Class: FallingJewel
 *
 * Parent: L<Jewel>
 *
 * This kind of jewel jumps around when it falls down
 * something, i.e. it is subject to gravity and moves around.
 * This is jewel type used when you eliminate an enemy
 * by means of a fireball.
 */


/**
 * Method: FallingJewel::new
 *
 *   new() → a_falling_jewel
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cFGoldpiece* p_fj = new cFGoldpiece(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_fj;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_fj->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_fj);

    return self;
}

void TSC::Scripting::Init_FallingJewel(mrb_state* p_state)
{
    struct RClass* p_rcFalling_Jewel = mrb_define_class(p_state, "FallingJewel", mrb_class_get(p_state, "Jewel"));
    MRB_SET_INSTANCE_TT(p_rcFalling_Jewel, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcFalling_Jewel, "initialize", Initialize, MRB_ARGS_NONE());
}
