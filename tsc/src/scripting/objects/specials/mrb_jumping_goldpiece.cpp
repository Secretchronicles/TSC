/***************************************************************************
 * mrb_jumping_goldpiece.cpp
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
 * Class: JumpingGoldpiece
 *
 * Parent: [Goldpiece](goldpiece.html)
 * {: .superclass}
 *
 * These goldpieces are used by coin boxes. You will barely
 * ever need them.
 */


/**
 * Method: JumpingGoldpiece::new
 *
 *   new() → a_jumping_goldpiece
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cJGoldpiece* p_jgp = new cJGoldpiece(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_jgp;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_jgp->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_jgp);

    return self;
}

void TSC::Scripting::Init_JumpingGoldpiece(mrb_state* p_state)
{
    struct RClass* p_rcJumping_Goldpiece = mrb_define_class(p_state, "JumpingGoldpiece", mrb_class_get(p_state, "Goldpiece"));
    MRB_SET_INSTANCE_TT(p_rcJumping_Goldpiece, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcJumping_Goldpiece, "initialize", Initialize, MRB_ARGS_NONE());
}
