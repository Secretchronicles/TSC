/***************************************************************************
 * mrb_cookie.cpp
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

#include "mrb_cookie.hpp"
#include "mrb_powerup.hpp"
#include "../../../level/level.hpp"
#include "../../../core/sprite_manager.hpp"
#include "../../../objects/powerup.hpp"

/**
 * Class: Cookie
 *
 * Parent: L<Powerup>
 *
 * The I<cookie> is a 3-live-up item that is typically used to make
 * the user aware he has successfully reached a particularly
 * hard-to-reach location. Do not overuse it! Use it sparely!
 *
 * Also note that the cookie is immune to gravity — wherever you
 * place it, it will just hang there and not fall down. This
 * facility can be used to innocently hide it behind clouds so
 * it looks like a part of the level background.
 */

using namespace TSC;
using namespace TSC::Scripting;


/**
 * Method: Cookie::new
 *
 *   new() → a_cookie
 *
 * Creates a new cookie powerup with the default values.
 */
static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    cMoon* p_moon = new cMoon(pActive_Level->m_sprite_manager);

    DATA_PTR(self) = p_moon;
    DATA_TYPE(self) = &rtTSC_Scriptable;

    // This is a generated object
    p_moon->Set_Spawned(true);

    // Let TSC manage the memory
    pActive_Level->m_sprite_manager->Add(p_moon);

    return self;
}

/**
 * Method: Cookie#activate!
 *
 *   activate!()
 *
 * Apply the item to Alex. Doing so will destroy the cookie in
 * any case, but note that applying it does not necessarily cause
 * it to have an effect on Alex. If Alex already has 99 lifes,
 * it will just cause points.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cMoon* p_moon = Get_Data_Ptr<cMoon>(p_state, self);
    p_moon->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Cookie(mrb_state* p_state)
{
    struct RClass* p_rcCookie = mrb_define_class(p_state, "Cookie", mrb_class_get(p_state, "Powerup"));
    MRB_SET_INSTANCE_TT(p_rcCookie, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcCookie, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcCookie, "activate!", Activate, MRB_ARGS_NONE());
}
