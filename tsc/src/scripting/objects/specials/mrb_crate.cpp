/***************************************************************************
 * mrb_crate.cpp
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

#include "../../../objects/crate.hpp"
#include "mrb_crate.hpp"
#include "../../events/event.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"

/**
 * Class: Crate
 *
 * Parent: [AnimatedSprite](animatedsprite.html)
 * {: .superclass}
 *
 * _Crates_ are heavy boxes Maryo can move around. They can be
 * used to unblock paths, kill enemies or just for fun.
 *
 * It is generally a bad idea to use a large number of stacked
 * crates, because they tend to behave in a weird manor if you
 * hit multiple of them at once. As long as you ensure only
 * one crate is hit at a time, everything should be fine.
 *
 * Maryo can’t move stacks of crates. Instead, the whole stack
 * will come down when you move a crate out of it; when crates
 * stand next to each other, Maryo is not able to move two
 * crates in a row.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Crate::new
 *
 *   new() → a_crate
 *
 * Create a new crate with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cCrate* p_crate = new cCrate(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_crate;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    p_crate->Set_Spawned(true);
    pActive_Level->m_sprite_manager->Add(p_crate);

    return self;
}

void TSC::Scripting::Init_Crate(mrb_state* p_state)
{
    struct RClass* p_rcCrate = mrb_define_class(p_state, "Crate", mrb_class_get(p_state, "AnimatedSprite"));
    MRB_SET_INSTANCE_TT(p_rcCrate, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcCrate, "initialize", Initialize, MRB_ARGS_NONE());
}
