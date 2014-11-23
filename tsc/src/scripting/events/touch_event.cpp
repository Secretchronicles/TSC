/***************************************************************************
 * touch_event.cpp
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

#include "touch_event.hpp"
#include "../objects/mrb_uids.hpp"
#include "../../objects/sprite.hpp"

using namespace TSC;
using namespace TSC::Scripting;

cTouch_Event::cTouch_Event(cSprite* p_collided)
{
    mp_collided = p_collided;
}

std::string cTouch_Event::Event_Name()
{
    return "touch";
}

cSprite* cTouch_Event::Get_Collided()
{
    return mp_collided;
}

void cTouch_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_state* p_state = p_mruby->Get_MRuby_State();

    // Look the UID of the colliding sprite up in the UIDS table
    // and grab the MRuby object representing it.
    mrb_value rcollided = mrb_funcall(p_state,
                                      mrb_obj_value(mrb_class_get(p_mruby->Get_MRuby_State(), "UIDS")),
                                      "[]",
                                      1,
                                      mrb_fixnum_value(mp_collided->m_uid));
    // Pass it to the callback
    mrb_funcall(p_state, callback, "call", 1, rcollided);
}
