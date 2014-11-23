/***************************************************************************
 * shoot_event.cpp
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

#include "shoot_event.hpp"

using namespace TSC;
using namespace TSC::Scripting;

cShoot_Event::cShoot_Event(std::string ball_type)
{
    m_ball_type = ball_type;
}

std::string cShoot_Event::Event_Name()
{
    return "shoot";
}

std::string cShoot_Event::Get_Ball_Type()
{
    return m_ball_type;
}

void cShoot_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_funcall(p_mruby->Get_MRuby_State(),
                callback,
                "call",
                1,
                str2sym(p_mruby->Get_MRuby_State(), (m_ball_type.c_str())));
}
