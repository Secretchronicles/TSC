/***************************************************************************
 * key_down_event.cpp
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

#include "key_down_event.hpp"

using namespace TSC;
using namespace TSC::Scripting;

cKeyDown_Event::cKeyDown_Event(std::string keyname)
{
    m_keyname = keyname;
}

std::string cKeyDown_Event::Get_Keyname()
{
    return m_keyname;
}

std::string cKeyDown_Event::Event_Name()
{
    return "key_down";
}

void cKeyDown_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_state* p_state = p_mruby->Get_MRuby_State();
    mrb_funcall(p_state,
                callback,
                "call",
                1,
                mrb_str_new_cstr(p_state, m_keyname.c_str()));
}
