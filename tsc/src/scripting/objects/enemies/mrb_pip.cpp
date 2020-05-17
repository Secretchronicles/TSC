/***************************************************************************
 * mrb_pip.cpp
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

#include "../../../enemies/pip.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../core/property_helper.hpp"
#include "mrb_enemy.hpp"
#include "mrb_pip.hpp"

/**
 * Class: Pip
 *
 * Parent: L<Enemy>
 *
 * I<Pip> is a large worm that slowly moves over the ground and should be
 * quite easy to avoid. Jumping with enough weight onto it will split
 * it up into two small worms, that are moving much faster and will
 * fling you away upwards when killed.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Pip::new
 *
 *   new() → a_pip
 *
 * Creates a new pip with the default values.
 */
static mrb_value Initialize(mrb_state* p_state,  mrb_value self)
{
    cPip* p_pip = new cPip(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_pip;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_pip->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_pip);

    return self;
}

void TSC::Scripting::Init_Pip(mrb_state* p_state)
{
    struct RClass* p_rcPip = mrb_define_class(p_state, "Pip", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcPip, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcPip, "initialize", Initialize, MRB_ARGS_NONE());
}
