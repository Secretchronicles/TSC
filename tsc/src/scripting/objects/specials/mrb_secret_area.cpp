/***************************************************************************
 * mrb_secret_area.cpp
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

#include "../../../objects/secret_area.hpp"
#include "mrb_secret_area.hpp"
#include "../../events/event.hpp"

/**
 * Class: SecretArea
 *
 * Parent: C<MovingSprite>
 *
 * I<Secret Areas> are places where the level designer has hidden special
 * objects, like life berries or a large amount of jewels. They are usually
 * difficult to find and display a notice saying that the player has
 * found a secret area when it is activated by entering it.
 *
 * This class cannot be instanciated; secret areas may only be created
 * in the level editor.
 *
 * =head2 Events
 *
 * =over
 *
 * =item [Activate]
 *
 * Issued when the player enters a secret area for the first time.
 * Subsequent entering does not cause this event to be emitted
 * anymore.
 *
 * =back
 */

using namespace TSC;
using namespace TSC::Scripting;

MRUBY_IMPLEMENT_EVENT(activate);

/**
 * Method: SecretArea#activate
 *
 *   activate()
 *
 * Activates the secret area as if Alex had entered into it.
 * Causes a subsequent Activate event.
 */
static mrb_value Activate(mrb_state* p_state, mrb_value self)
{
    cSecret_Area* p_secarea = Get_Data_Ptr<cSecret_Area>(p_state, self);
    p_secarea->Activate();

    return mrb_nil_value();
}

/**
 * Method: SecretArea#activated?
 *
 * Returns true if the secret area was activated already,
 * false otherwise.
 */
static mrb_value Activated(mrb_state* p_state, mrb_value self)
{
    cSecret_Area* p_secarea = Get_Data_Ptr<cSecret_Area>(p_state, self);

    if (p_secarea->m_activated)
        return mrb_true_value();
    else
        return mrb_false_value();
}

void TSC::Scripting::Init_SecretArea(mrb_state* p_state)
{
    struct RClass* p_rcSecret_Area = mrb_define_class(p_state, "SecretArea", mrb_class_get(p_state, "MovingSprite"));
    MRB_SET_INSTANCE_TT(p_rcSecret_Area, MRB_TT_DATA);

    // Forbid creating new instances of SecretArea
    mrb_undef_class_method(p_state, p_rcSecret_Area, "new");

    // Normal methods
    mrb_define_method(p_state, p_rcSecret_Area, "activate", Activate, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSecret_Area, "activated?", Activated, MRB_ARGS_NONE());
    mrb_define_method(p_state, p_rcSecret_Area, "on_activate", MRUBY_EVENT_HANDLER(activate), MRB_ARGS_BLOCK());
}
