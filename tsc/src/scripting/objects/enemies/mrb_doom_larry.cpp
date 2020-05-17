/***************************************************************************
 * mrb_doom_larry.cpp
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

#include "../../../enemies/doom_larry.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_doom_larry.hpp"
#include "mrb_enemy.hpp"

/**
 * Class: DoomLarry
 *
 * Parent: L<Larry>
 *
 * While you may not be too worried about Larry, you do need to take care with I<Doom_Larry>.  When it
 * explodes, it will instantly kill both Alex and all enemies nearby.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: DoomLarry::new
 *
 *   new() → a_doom_larry
 *
 * Creates a new doom larry.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cDoomLarry* p_dl = new cDoomLarry(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_dl;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_dl->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_dl);
    return self;
}

void TSC::Scripting::Init_DoomLarry(mrb_state* p_state)
{
    struct RClass* p_rcDoomLarry = mrb_define_class(p_state, "DoomLarry", mrb_class_get(p_state, "Enemy"));
    MRB_SET_INSTANCE_TT(p_rcDoomLarry, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcDoomLarry, "initialize", Initialize, MRB_ARGS_NONE());
}
