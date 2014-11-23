/***************************************************************************
 * level_load_event.cpp
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

#include "level_load_event.hpp"
#include "../../core/property_helper.hpp"

using namespace TSC;
using namespace TSC::Scripting;

cLevel_Load_Event::cLevel_Load_Event(std::string save_data)
{
    m_save_data = save_data;
}

std::string cLevel_Load_Event::Event_Name()
{
    return "load";
}

std::string cLevel_Load_Event::Get_Save_Data()
{
    return m_save_data;
}

void cLevel_Load_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    mrb_state* p_state = p_mruby->Get_MRuby_State();
    mrb_value mod_json = mrb_const_get(p_state, mrb_obj_value(p_state->object_class), mrb_intern_cstr(p_state, "JSON"));

    // Deserialise from the JSON representation into an MRuby hash
    mrb_value target_str = mrb_str_new_cstr(p_state, m_save_data.c_str());
    mrb_value result = mrb_funcall(p_state, mod_json, "parse", 1, target_str);

    // Bad things happened
    if (p_state->exc) {
        mrb_value exception = mrb_obj_value(p_state->exc);
        std::string text = mrb_string_value_ptr(p_state, mrb_funcall(p_state, exception, "message", 0));

        std::cerr << "Warning: Failed to deserialize JSON representation from savegame: ";
        std::cerr << text << std::endl;
        std::cerr << std::endl << "(Skipping all handlers registered to Level.on_load)" << std::endl;
        return;
    }

    mrb_funcall(p_state, callback, "call", 1, result);
}
