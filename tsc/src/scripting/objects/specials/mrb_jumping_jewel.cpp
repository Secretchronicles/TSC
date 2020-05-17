/***************************************************************************
 * mrb_jumping_jewel.cpp
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
 * Class: JumpingJewel
 *
 * Parent: L<Jewel>
 *
 * These jewels are used by jewel boxes. You will barely
 * ever need them.
 */


/**
 * Method: JumpingJewel::new
 *
 *   new() → a_jumping_jewel
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cJGoldpiece* p_jj = new cJGoldpiece(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_jj;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_jj->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_jj);

    return self;
}

void TSC::Scripting::Init_JumpingJewel(mrb_state* p_state)
{
    struct RClass* p_rcJumping_Jewel = mrb_define_class(p_state, "JumpingJewel", mrb_class_get(p_state, "Jewel"));
    MRB_SET_INSTANCE_TT(p_rcJumping_Jewel, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcJumping_Jewel, "initialize", Initialize, MRB_ARGS_NONE());
}
