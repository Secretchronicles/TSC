/***************************************************************************
 * level_save_load_event.cpp
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

#include "level_save_load_event.hpp"

using namespace TSC;
using namespace TSC::Scripting;

/**
 * `is_save' specifies if this is a save operation (true) or
 * a load operation (false).
 */
cLevel_SaveLoad_Event::cLevel_SaveLoad_Event(bool is_save)
{
    m_is_save = is_save;
}

std::string cLevel_SaveLoad_Event::Event_Name()
{
    return "save_load";
}

void cLevel_SaveLoad_Event::Run_MRuby_Callback(cMRuby_Interpreter* p_mruby, mrb_value callback)
{
    // For each handler registered, create one instance of ScriptData
    // and store it in the m_cpp_storage vector. Script developers
    // interact with `storage' (type Level::SaveSerializer) as the
    // mruby interface.
    mrb_state* p_state = p_mruby->Get_MRuby_State();
    mrb_value storage  = mrb_obj_new(p_state, mrb_class_get(p_state, "SaveSerializer"), 0, nullptr);
    mrb_int key        = p_mruby->Protect_From_GC(storage); // Ensure `storage' is not garbage-collected during callback run

    if (m_is_save) {
        mrb_funcall(p_state, callback, "call", 2, storage, mrb_true_value());
        Script_Data* p_data = Get_Data_Ptr<Script_Data>(p_state, storage);
        m_cpp_storage.push_back(*p_data);
    }
    else {
        // Exchange the wrapped C++ object with the one set with
        // Set_Storage() to make the loaded data vailable.
        if (m_cpp_storage.size() > 0) {
            delete Get_Data_Ptr<Script_Data>(p_state, storage);
            DATA_PTR(storage) = new Script_Data(m_cpp_storage.front());
            m_cpp_storage.erase(m_cpp_storage.begin());
        }
        else {
            std::cerr << "Warning: Malformed script storage in save file, providing empty storage for this handler" << std::endl;
        }

        mrb_funcall(p_state, callback, "call", 2, storage, mrb_false_value());
    }

    // rtTSC_Scriptable, which is used for Level::SaveSerializer,
    // has no free function defined, so manual deletion is required.
    delete Get_Data_Ptr<Script_Data>(p_state, storage);
    DATA_PTR(storage) = nullptr;
    p_mruby->Unprotect_From_GC(key); // GC can collect it now
}

/**
 * Returns the accumulated stored script data. Each event handler
 * for the SaveLoad event causes one entry in this vector.
 * Only useful for a save action after the event was fired.
 */
std::vector<Script_Data> cLevel_SaveLoad_Event::Get_Storage()
{
    return m_cpp_storage;
}

/**
 * Set the accumulated stored script data. Each event handler for the
 * SaveLoad event receives one entry from this vector. Only useful for
 * a load action before the event was fired.
 */
void cLevel_SaveLoad_Event::Set_Storage(const std::vector<Script_Data>& storage)
{
    m_cpp_storage = storage;
}
