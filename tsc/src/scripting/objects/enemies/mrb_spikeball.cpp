/***************************************************************************
 * mrb_spikeball.cpp
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

#include "../../../enemies/spikeball.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_spikeball.hpp"

/**
 * Class: Spikeball
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * Be careful when using _Spikeballs_. They are invincible in normal
 * gameplay and typically used in levels not intended for beginners to
 * make reaching specific platforms difficult or prevent the player from
 * staying at a place for too long. Although it _is_ possible to defy them
 * using the star powerup, this should really be the exception. They gain
 * their character by their invicibility, and combining them with stars too
 * often makes them lose their point. Instead, if you want to block a passage
 * in a way Maryo can’t get through without a star, use eatos (and probably remove
 * their ice weakness using scripting).
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Spikeball::new
 *
 *   new() → a_spikeball
 *
 * Creates a new spikeball with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cSpikeball* p_spikeball = new cSpikeball(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_spikeball;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_spikeball->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_spikeball);

    return self;
}

// TODO: Currently there is only the grey spikeball. Hence exposing
// cSpikeball::Set_Color() to the mruby scripting interface is not
// useful yet.

void TSC::Scripting::Init_Spikeball(mrb_state* p_state)
{
    struct RClass* p_rcSpikeball = mrb_define_class(p_state, "Spikeball", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcSpikeball, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcSpikeball, "initialize", Initialize, MRB_ARGS_NONE());
}
