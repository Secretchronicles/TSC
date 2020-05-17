/***************************************************************************
 * mrb_lemon.cpp
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

#include "mrb_lemon.hpp"
#include "mrb_powerup.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../objects/star.hpp"

/**
 * Class: Lemon
 *
 * Parent: L<Powerup>
 *
 * I<Lemons> are the most powerful powerups in the game. Alex gains the
 * ability to jump higher, gets resistant against all kinds of enemies,
 * etc. Lemons wildly jump around and are hard to catch. Even if catched,
 * its effect only lasts a fixed period of time (which is hardcoded into
 * TSC, thus not configurable via the scripting API).
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Lemon::new
 *
 *   new() → a_lemon
 *
 * Creates a new lemon powerup with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cjStar* p_star = new cjStar(pActive_Level->m_sprite_manager);

    DATA_PTR(self) = p_star;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_star->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_star);

    return self;
}

/**
 * Method: Lemon#activate!
 *
 *   activate!()
 *
 * Apply the item to Alex. Doing so will destroy the lemon. If
 * Alex already has lemon effect, the lemon effect time is reset
 * to 0.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cjStar* p_star = Get_Data_Ptr<cjStar>(p_state, self);
    p_star->Activate();
    return mrb_nil_value();
}

/**
 * Method: Lemon#glimming=
 *
 *   glimming=( bool ) → bool
 *
 * TODO: Docs.
 */
static mrb_value Set_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    mrb_bool glim;
    mrb_get_args(p_state, "b", &glim);

    cjStar* p_star = Get_Data_Ptr<cjStar>(p_state, self);
    p_star->m_glim_mod = glim;

    return mrb_bool_value(glim);
}

/**
 * Method: Lemon#glimming?
 *
 *   glimming?() → true or false
 *
 * TODO: Docs.
 */
static mrb_value Get_Glim_Mode(mrb_state* p_state, mrb_value self)
{
    cjStar* p_star = Get_Data_Ptr<cjStar>(p_state, self);
    return mrb_bool_value(p_star->m_glim_mod);
}

void TSC::Scripting::Init_Lemon(mrb_state* p_state)
{
    struct RClass* p_rcLemon = mrb_define_class(p_state, "Lemon", mrb_class_get(p_state, "Powerup"));
    MRB_SET_INSTANCE_TT(p_rcLemon, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcLemon, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLemon, "activate!", Activate, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcLemon, "glimming=", Set_Glim_Mode, MRB_ARGS_REQ(1));
    mrb_define_method(p_state, p_rcLemon, "glimming?", Get_Glim_Mode, MRB_ARGS_NONE());
}
