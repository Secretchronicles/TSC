/***************************************************************************
 * mrb_larry.cpp
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

#include "../../../enemies/larry.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_enemy.hpp"

/**
 * Class: Larry
 *
 * Parent: [Enemy](enemy.html)
 * {: .superclass}
 *
 * If you just here some loud "BOOOM!", then you have met _Larry_.
 * On exploding, he kills all enemies (and Maryo) nearby. Really
 * dangerous is a situation with multiple larrys, which can result
 * in a giant chain explosion.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Larry::new
 *
 *   new() → a_larry
 *
 * Creates a new larry.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cLarry* p_larry = new cLarry(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_larry;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_larry->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_larry);
    return self;
}

void TSC::Scripting::Init_Larry(mrb_state* p_state)
{
    struct RClass* p_rcLarry = mrb_define_class(p_state, "Larry", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcLarry, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcLarry, "initialize", Initialize, MRB_ARGS_NONE());
}
