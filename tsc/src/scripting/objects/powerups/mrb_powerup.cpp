/***************************************************************************
 * mrb_powerup.cpp
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

#include "mrb_powerup.hpp"
#include "../sprites/mrb_moving_sprite.hpp"
#include "../../../objects/powerup.hpp"
#include "../../events/event.hpp"

/**
 * Class: Powerup
 *
 * Parent: L<MovingSprite>
 *
 * I<Powerups> are goodies (well, or badies) that affect Alex’s
 * state somehow. This class is the baseclass for all powerups.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Activate]
 *
 * This event is triggered when the powerup is applied to
 * the player. Do not apply other powerups to the player
 * inside the event handler for this, this will cause
 * undefined behaviour.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(activate);


static mrb_value Initialize(mrb_state* p_state, mrb_value self)
{
    mrb_raise(p_state, MRB_NOTIMP_ERROR(p_state), "Cannot create instances of this class.");
    return self; // Not reached
}

/**
 * Method: Powerup#activate!
 *
 *   activate!()
 *
 * Apply the powerup to the player. This method is not really
 * useful on itself, it is overridden in the subclasses to
 * do something meaningful. Currently, it just issues
 * the C<Activate> event.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cPowerUp* p_powerup = Get_Data_Ptr<cPowerUp>(p_state, self);
    p_powerup->Activate();
    return mrb_nil_value();
}

void TSC::Scripting::Init_Powerup(mrb_state* p_state)
{
    struct RClass* p_rcPowerup = mrb_define_class(p_state, "Powerup", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcPowerup, MRB_TT_DATA);

    mrb_define_method(p_state, p_rcPowerup, "initialize", Initialize, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcPowerup, "activate!", Activate, MRB_ARGS_NONE());

    mrb_define_method(p_state, p_rcPowerup, "on_activate", MRUBY_EVENT_HANDLER(activate), MRB_ARGS_BLOCK());
}
