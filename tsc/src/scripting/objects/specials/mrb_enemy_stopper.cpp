/***************************************************************************
 * mrb_enemy_stopper.cpp
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

#include "../../../objects/enemystopper.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../level/level.hpp"
#include "mrb_enemy_stopper.hpp"
#include "../sprites/mrb_animated_sprite.hpp"

/**
 * Class: EnemyStopper
 *
 * Parent: [AnimatedSprite](animatedsprite.html)
 * {: .superclass}
 *
 * An _enemy stopper_ acts as a barrier for most enemies
 * in TSC. They are not able to cross this invisible barrier
 * and will turn around when they hit it.
 *
 * Enemies that do not walk as usually (for example
 * [gees](gee.html) are not affected by this barrier.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: EnemyStopper::new
 *
 *   new() → an_enemy_stopper
 *
 * Creates a new instance of this class.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cEnemyStopper* p_es = new cEnemyStopper(pActive_Level->m_sprite_manager);
    DATA_PTR(self) = p_es;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_es->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_es);

    return self;
}

void TSC::Scripting::Init_EnemyStopper(mrb_state* p_state)
{
    struct RClass* p_rcEnemy_Stopper = mrb_define_class(p_state, "EnemyStopper", mrb_class_get(p_state, "AnimatedSprite"));
    MRB_SET_INSTANCE_TT(p_rcEnemy_Stopper, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcEnemy_Stopper, "initialize", Initialize, MRB_ARGS_NONE());
}
