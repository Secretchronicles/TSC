/***************************************************************************
 * mrb_fireberry.cpp
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

#include "mrb_fireberry.hpp"
#include "mrb_powerup.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../objects/powerup.hpp"

/**
 * Class: Fireberry
 *
 * Parent: L<Powerup>
 *
 * I<Fireberries> are powerups that grant Alex the skill to throw
 * fireballs agains enemies. Fireberries don’t move by default.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Fireberry::new
 *
 *   new() → a_fireberry
 *
 * Creates a new fireberry powerup with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cFirePlant* p_fireberry = new cFirePlant(pActive_Level->m_sprite_manager);

    DATA_PTR(self) = p_fireberry;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_fireberry->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_fireberry);

    return self;
}

/**
 * Method: Fireberry#activate!
 *
 *   activate!()
 *
 * Apply the item to Alex. Doing so will destroy the fireberry in
 * any case, but note that applying it does not necessarily cause
 * it to have an effect on Alex (use L<LevelPlayer#type=> for
 * that). Instead, it may be stored in the item box, or even
 * just have no effect if that is already full.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cFirePlant* p_fireberry = Get_Data_Ptr<cFirePlant>(p_state, self);
    p_fireberry->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Fireberry(mrb_state* p_state)
{
    struct RClass* p_rcFireberry = mrb_define_class(p_state, "Fireberry", mrb_class_get(p_state, "Powerup"));
    MRB_SET_INSTANCE_TT(p_rcFireberry, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcFireberry, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcFireberry, "activate!", Activate, MRB_ARGS_NONE());
}
